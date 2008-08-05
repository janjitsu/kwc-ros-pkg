/*
 * Copyright (c) 2008, Maxim Likhachev
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 * 
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of the University of Pennsylvania nor the names of its
 *       contributors may be used to endorse or promote products derived from
 *       this software without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */
#include "../headers.h"

#if MEM_CHECK == 1
void DisableMemCheck()
{
// Get the current state of the flag
// and store it in a temporary variable
int tmpFlag = _CrtSetDbgFlag( _CRTDBG_REPORT_FLAG );

// Turn On (OR) - All freed memory is re-initialized with xDD
tmpFlag |= _CRTDBG_DELAY_FREE_MEM_DF;

// Turn Off (AND) - memory checking is disabled for future allocations
tmpFlag &= ~_CRTDBG_ALLOC_MEM_DF;

// Set the new state for the flag
_CrtSetDbgFlag( tmpFlag );

}

void EnableMemCheck()
{
// Get the current state of the flag
// and store it in a temporary variable
int tmpFlag = _CrtSetDbgFlag( _CRTDBG_REPORT_FLAG );

// Turn On (OR) - All freed memory is re-initialized with xDD
tmpFlag |= _CRTDBG_DELAY_FREE_MEM_DF;

// Turn On (OR) - memory checking is enabled for future allocations
tmpFlag |= _CRTDBG_ALLOC_MEM_DF;

// Set the new state for the flag
_CrtSetDbgFlag( tmpFlag );

}
#endif

void checkmdpstate(CMDPSTATE* state)
{
#if DEBUG == 0
	printf("ERROR: checkMDPstate is too expensive for not in DEBUG mode\n");
	exit(1);
#endif

	for(int aind = 0; aind < (int)state->Actions.size(); aind++)
	{
		for(int aind1 = 0; aind1 < (int)state->Actions.size(); aind1++)
		{
			if(state->Actions[aind1]->ActionID == state->Actions[aind]->ActionID &&
				aind1 != aind)
			{
				printf("ERROR in CheckMDP: multiple actions with the same ID exist\n");
				exit(1);
			}
		}
		for(int sind = 0; sind < (int)state->Actions[aind]->SuccsID.size(); sind++)
		{	
			for(int sind1 = 0; sind1 < (int)state->Actions[aind]->SuccsID.size(); sind1++)
			{
				if(state->Actions[aind]->SuccsID[sind] == state->Actions[aind]->SuccsID[sind1] &&
					sind != sind1)
				{
					printf("ERROR in CheckMDP: multiple outcomes with the same ID exist\n");
					exit(1);
				}
			}
		}		
	}
}


void CheckMDP(CMDP* mdp)
{
	for(int i = 0; i < (int)mdp->StateArray.size(); i++)
	{
		checkmdpstate(mdp->StateArray[i]);
	}
}




void PrintMatrix(int** matrix, int rows, int cols, FILE* fOut)
{
	for(int r = 0; r < rows; r++)
	{
		for(int c = 0; c < cols; c++)
		{
			fprintf(fOut, "%d ", matrix[r][c]);
		}
		fprintf(fOut, "\n");
	}
}


//return true if there exists a path from sourcestate to targetstate and false otherwise
bool PathExists(CMDP* pMarkovChain, CMDPSTATE* sourcestate, CMDPSTATE* targetstate)
{
	CMDPSTATE* state;
	vector<CMDPSTATE*> WorkList;
	int i;
	bool *bProcessed = new bool [pMarkovChain->StateArray.size()];
	bool bFound = false;

	//insert the source state
	WorkList.push_back(sourcestate);
	while((int)WorkList.size() > 0)
	{
		//get the state and its info
		state = WorkList[WorkList.size()-1];
		WorkList.pop_back();

		//Markov Chain should just contain a single policy
		if((int)state->Actions.size() > 1)
		{
			printf("ERROR in PathExists: Markov Chain is a general MDP\n");
			exit(1);
		}

		if(state == targetstate)
		{
			//path found
			bFound = true;
			break;
		}

		//otherwise just insert policy successors into the worklist unless it is a goal state
		for(int sind = 0; (int)state->Actions.size() != 0 && sind < (int)state->Actions[0]->SuccsID.size(); sind++)
		{
			//get a successor
			for(i = 0; i < (int)pMarkovChain->StateArray.size(); i++)
			{
				if(pMarkovChain->StateArray[i]->StateID == state->Actions[0]->SuccsID[sind])
					break;
			}
			if(i == (int)pMarkovChain->StateArray.size())
			{	
				printf("ERROR in PathExists: successor is not found\n");
				exit(1);
			}
			CMDPSTATE* SuccState = pMarkovChain->StateArray[i];
					
			//insert at the end of list if not there or processed already
			if(!bProcessed[i])
			{
				bProcessed[i] = true;
				WorkList.push_back(SuccState);
			}
		} //for successors
	}//while WorkList is non empty

	delete [] bProcessed;

	return bFound;
}	

int ComputeNumofStochasticActions(CMDP* pMDP)
{
	int i;
	int nNumofStochActions = 0;
	printf("ComputeNumofStochasticActions...\n");

	for(i = 0; i < (int)pMDP->StateArray.size(); i++)
	{
		for(int aind = 0; aind < (int)pMDP->StateArray[i]->Actions.size(); aind++)
		{
			if((int)pMDP->StateArray[i]->Actions[aind]->SuccsID.size() > 1)
				nNumofStochActions++;
		}
	}
	printf("done\n");

	return nNumofStochActions;
}


void EvaluatePolicy(CMDP* PolicyMDP, int StartStateID, int GoalStateID,
					double* PolValue, bool *bFullPolicy, double* Pcgoal, int *nMerges,
					bool* bCycles)
{
	int i, j, startind=-1;
	double delta = INFINITECOST;
	double mindelta = 0.1;

	*Pcgoal = 0;
	*nMerges = 0;

	printf("Evaluating policy...\n");

	//create and initialize values
	double* vals = new double [PolicyMDP->StateArray.size()];
	double* Pcvals = new double [PolicyMDP->StateArray.size()];
	for(i = 0; i < (int)PolicyMDP->StateArray.size(); i++)
	{
		vals[i] = 0;
		Pcvals[i] = 0;

		//remember the start index
		if(PolicyMDP->StateArray[i]->StateID == StartStateID)
		{
			startind = i;
			Pcvals[i] = 1;
		}
	}

	//initially assume full policy
	*bFullPolicy = true;
	bool bFirstIter = true;
	while(delta > mindelta)
	{
		delta = 0;
		for(i = 0; i < (int)PolicyMDP->StateArray.size(); i++)
		{
			//get the state
			CMDPSTATE* state = PolicyMDP->StateArray[i];

			//do the backup for values
			if(state->StateID == GoalStateID)
			{
				vals[i] = 0;
			}
			else if((int)state->Actions.size() == 0)
			{
				*bFullPolicy = false;
				vals[i] = UNKNOWN_COST;
				*PolValue = vals[startind];
				return;
			}
			else
			{
				//normal backup
				CMDPACTION* action = state->Actions[0];

				//do backup
				double Q = 0;
				for(int oind = 0; oind < (int)action->SuccsID.size(); oind++)
				{
					//get the state
					for(j = 0; j < (int)PolicyMDP->StateArray.size(); j++)
					{	
						if(PolicyMDP->StateArray[j]->StateID == action->SuccsID[oind])
							break;
					}
					if(j == (int)PolicyMDP->StateArray.size())
					{
						printf("ERROR in EvaluatePolicy: incorrect successor %d\n", 
							action->SuccsID[oind]);
						exit(1);
					}
					Q += action->SuccsProb[oind]*(vals[j] + action->Costs[oind]);
				}

				if(vals[i] > Q)
				{
					printf("ERROR in EvaluatePolicy: val is decreasing\n"); 
					exit(1);
				}

				//update delta
				if(delta < Q - vals[i])
					delta = Q-vals[i];

				//set the value
				vals[i] = Q;
			}

			//iterate through all the predecessors and compute Pc
			double Pc = 0;
			//go over all predecessor states
			int nMerge = 0;
			for(j = 0; j < (int)PolicyMDP->StateArray.size(); j++)
			{
				for(int oind = 0; (int)PolicyMDP->StateArray[j]->Actions.size() > 0 && 
					oind <  (int)PolicyMDP->StateArray[j]->Actions[0]->SuccsID.size(); oind++)
				{
					if(PolicyMDP->StateArray[j]->Actions[0]->SuccsID[oind] == state->StateID)
					{
						//process the predecessor
						double PredPc = Pcvals[j];
						double OutProb = PolicyMDP->StateArray[j]->Actions[0]->SuccsProb[oind];
				
						//accumulate into Pc
						Pc = Pc + OutProb*PredPc;
						nMerge++;

						//check for cycles
						if(bFirstIter && !(*bCycles))
						{
							if(PathExists(PolicyMDP, state, PolicyMDP->StateArray[j]))
								*bCycles = true;
						}
					}
				}
			}
			if(bFirstIter && state->StateID != GoalStateID && nMerge > 0)
				*nMerges += (nMerge-1);

			//assign Pc
			if(state->StateID != StartStateID)
				Pcvals[i] = Pc;

			if(state->StateID == GoalStateID)
				*Pcgoal = Pcvals[i];
		} //over  states
		bFirstIter = false;
	} //until delta small

	*PolValue = vals[startind];
	
	printf("done\n");
}

