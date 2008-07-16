#include "Components.hh"
#include "PlanDatabase.hh"
#include "Token.hh"
#include "TokenVariable.hh"
#include "TemporalAdvisor.hh"
#include "DbCore.hh"
#include "Constraints.hh"
#include "ConstraintLibrary.hh"
#include "Timeline.hh"
#include "Agent.hh"
#include "GoalManager.hh"


#include <gdk-pixbuf/gdk-pixbuf.h>
#include <libstandalone_drivers/plan.h>

#include <math.h>

namespace TREX{

  GoalsOnlyFilter::GoalsOnlyFilter(const TiXmlElement& configData): FlawFilter(configData, true) {}

  bool GoalsOnlyFilter::test(const EntityId& entity){
    checkError(TokenId::convertable(entity), "Invalid configuration for " << entity->toString());

    TokenId token(entity);
    return !token->getState()->baseDomain().isMember(Token::REJECTED);
  }

  NoGoalsFilter::NoGoalsFilter(const TiXmlElement& configData): FlawFilter(configData, true) {}

  bool NoGoalsFilter::test(const EntityId& entity){
    checkError(TokenId::convertable(entity), "Invalid configuration for " << entity->toString());

    TokenId token(entity);
    return token->getState()->baseDomain().isMember(Token::REJECTED);
  }


  /**
   * @brief Constructor will read xml configuration data as needed. Can extend to include a map input source for example.
   * @todo Include a map input file as a configuration argument
   */
  GoalManager::GoalManager(const TiXmlElement& configData)
    : OpenConditionManager(configData),
      m_maxIterations(1000),
      m_plateau(5),
      m_positionSourceCfg(""),
      m_state(STATE_DONE) {
    // Read configuration parameters to override defaults


    //Allocate the cost estimator.
    for (TiXmlElement* child = configData.FirstChildElement(); child;
	 child = child->NextSiblingElement()) {
      if (std::string(child->Value()) == "CostEstimator") {
	assertTrue(m_costEstimator == CostEstimatorId::noId(),
		   "Your GoalManager has more than one cost estimator.");
	m_costEstimator = SOLVERS::Component::AbstractFactory::allocate(*child);
      }
    }
    assertTrue(m_costEstimator != CostEstimatorId::noId(),
	       "Your GoalManager has no cost estimator.");


    // MAX ITERATIONS
    const char * maxI = configData.Attribute(CFG_MAX_ITERATIONS().c_str());
    if(maxI != NULL)
      m_maxIterations = atoi(maxI);

    // PLATEAU
    const char * plateau = configData.Attribute(CFG_PLATEAU().c_str());
    if(plateau != NULL)
      m_plateau = atoi(plateau);

    // POSITION SOURCE
    const char * positionSrc = configData.Attribute(CFG_POSITION_SOURCE().c_str());
    if(positionSrc != NULL)
      m_positionSourceCfg = LabelStr(positionSrc);
  }

  void GoalManager::step() {
    if (noMoreFlaws()) {
      return;
    }  

    if(m_state == STATE_REQUIRE_PLANNING){
      generateInitialSolution();
      
      m_iteration = m_watchDog = 0;
    }
    
    m_state = STATE_PLANNING;
    if(m_iteration < m_maxIterations && m_watchDog < m_plateau) {    
      // Update counters to handle termination
      m_iteration++;
      m_watchDog++;
      
      // Get best neighbor
      TokenId delta;
      GoalManager::SOLUTION candidate;
      selectNeighbor(candidate, delta);
      
      // In the event that the candidate is the same solution, we have hit the end of exploration unless
      // we escape somehow. The algorithm does not include such random walks to explore beyond the immediate
      // neighborhood
      if(candidate == m_currentSolution) {
	m_state = STATE_DONE;
      } else {
	// Promote if not worse. Allos for some exploration
	int result = compare(candidate, m_currentSolution);
	if(result >= 0) {
	  // If a token was removed, insert into ommitted token list, and opposite if appended
	  if(m_currentSolution.size() > candidate.size())
	    m_ommissions.insert(delta);
	  else if(m_currentSolution.size() < candidate.size())
	    m_ommissions.erase(delta);
	  
	  // Update current solution Values
	  m_currentSolution = candidate;
	  
	  // Only pet the watchdog if we have improved the score. This allows us to terminate when
	  // we have plateaued
	  if(result == 1)
	    m_watchDog = 0;
	  
	  debugMsg("GoalManager:search", "Switching to new solution: " << toString(m_currentSolution));
	} else {
	  m_state = STATE_DONE;
	}
      }
    } else {
      m_state = STATE_DONE;
    }
    if ( m_state == STATE_DONE) {
      debugMsg("GoalManager:search", "Returning: " << toString(m_currentSolution));
    }
  }


  bool GoalManager::noMoreFlaws() {
    return m_state == STATE_DONE;
  }

  bool GoalManager::isNextToken(TokenId token) {
    if (!noMoreFlaws()) {
      return false;
    }
    TokenId nextGoal = TokenId::noId();
    for(SOLUTION::const_iterator it = m_currentSolution.begin(); it != m_currentSolution.end(); ++it){
      TokenId t = *it;
      if(t->isInactive()){
	nextGoal = t;
	break;
      }
    }
    return (nextGoal == token);
  }




  /**
   * @brief When a new flaw is added we will re-evaluate all the options. This is accomplished
   * by incrementing a cycle count which makes existing solution stale.
   */
  void GoalManager::addFlaw(const TokenId& token){
    OpenConditionManager::addFlaw(token);

    if(token->getState()->baseDomain().isMember(Token::REJECTED)){
      debugMsg("GoalManager:addFlaw", token->toString());
      m_state = STATE_REQUIRE_PLANNING;
    }
  }

  void GoalManager::removeFlaw(const TokenId& token){
    OpenConditionManager::removeFlaw(token);

    // If the flaw was rejected, come up with a revised solution to get better utility
    if(token->isRejected()) {
      m_state = STATE_REQUIRE_PLANNING;
    }

    // If the token is active, remove it from the current solution if present
    if(token->isActive()) {
      SOLUTION::iterator it = m_currentSolution.begin();
      while(it != m_currentSolution.end()){
	TokenId t = *it;
	if(t == token){
	  m_currentSolution.erase(it);
	  break;
	}
	++it;
      }
    }

    // Clear from ommittedTokens
    m_ommissions.erase(token);
  }

  void GoalManager::handleInitialize(){
    static const LabelStr sl_nullLabel("");
    OpenConditionManager::handleInitialize();
    m_state = STATE_REQUIRE_PLANNING;
    if(m_positionSourceCfg != sl_nullLabel){
      debugMsg("GoalManager:handleInitialize", "Looking to source position information from '" << m_positionSourceCfg.toString() << "'");
      m_positionSource = getPlanDatabase()->getObject(m_positionSourceCfg);
    }
  }

  /**
   * @brief Generates a simple seed solution
   */
  void GoalManager::generateInitialSolution(){
    debugMsg("GoalManager:generateInitialSolution", "Resetting solution");
    
    m_currentSolution.clear();
    m_ommissions.clear();

    // Set the initial conditions since the problem may have moved on
    setInitialConditions();

    // The empty solution is the default solution
    IteratorId it = OpenConditionManager::createIterator();
    while(!it->done()){
      TokenId goal = (TokenId) it->next();
      checkError(goal->getMaster().isNoId(), goal->toString());
      m_currentSolution.push_back(goal);
    }

    delete (FlawIterator*) it;
  }


  GoalManager::~GoalManager(){
    m_costEstimator.release();
  }


  /**
   * @brief For now we use euclidean distance. This is where map integration comes in.
   */
  double GoalManager::computeDistance(const Position& p1, const Position& p2){
    return m_costEstimator->computeDistance(p1, p2);
  }

  /**
   * @brief For now, this is going to be all about speed, distance and time
   */
  bool GoalManager::evaluate(const SOLUTION& s, double& cost, double& utility){
    static const int MAX_PRIORITY = 5;
    cost = 0;
    utility = 0;
    TokenId predecessor;
    unsigned int numConflicts(0);

    static const LabelStr INACTIVE("Navigator.Inactive");
    double pathLength = 0;
    Position currentPosition = getCurrentPosition();


    for(SOLUTION::const_iterator it = s.begin(); it != s.end(); ++it){
      TokenId candidate = *it;
      checkError(candidate.isId() && candidate->getMaster().isNoId(), candidate->toString());

      // Utility is 10 to the power of the priority
      utility += pow(10.0, (MAX_PRIORITY - getPriority(candidate)));

      // Check if there is a conflict
      if(predecessor.isId() && !getPlanDatabase()->getTemporalAdvisor()->canPrecede(predecessor, candidate))
	numConflicts++;

      Position nextPosition = getPosition(candidate);

      pathLength += computeDistance(currentPosition, nextPosition);
      predecessor = candidate;
      currentPosition = nextPosition;
    }

    // Priority is to remove conflicts so a much higher weight is given to that
    cost = (pathLength / getSpeed()) + (numConflicts * pow(10.0, MAX_PRIORITY));

    // Finally, feasibility is based on cost being within available budget for time.
    // That budget is defined by the look ahead window of the solver
    return cost <= m_timeBudget;
  }

  std::string GoalManager::toString(const SOLUTION& s){
    double cost, utility;
    evaluate(s, cost, utility);

    std::stringstream ss;
    for(SOLUTION::const_iterator it = s.begin(); it != s.end(); ++it){
      TokenId t = *it;
      ss << t->getKey() << ":";
    }
    ss << "(" << cost << "/" << utility << ")";
    return ss.str();
  }

  /**
   * @brief Get the best neighbor for the current solution. The neighborhood is the set of solutions
   * within a single operation from the current solution. The operators are:
   * 1. insert
   * 2. swap
   * 3. remove
   * There are O(n^2) neigbors
   *
   * @note There is alot more we can do to exploit temporal constraints and evaluate feasibility. We are not including
   * deadlines and we are not factoring in the possibility that insertion in the solution imposes implied temporal constraints.
   * @todo Cache feasibility of current solution
   */
  void GoalManager::selectNeighbor(GoalManager::SOLUTION& s, TokenId& delta){
    checkError(!m_currentSolution.empty() || !m_ommissions.empty(), "There must be something to do");
    s = m_currentSolution;

    // Feasibility can be used to avoid moves that are silly. We should be able to cache the feasiblility of the current solution
    // rather than compute anew.
    double d1, d2;
    bool feasible = evaluate(m_currentSolution, d1, d2);
    delta = TokenId::noId();

    // Try insertions - could skip if current solution is infeasible.
    if(feasible){
      for(TokenSet::const_iterator it = m_ommissions.begin(); it != m_ommissions.end(); ++it){
	TokenId t = *it;
	for (unsigned int i = 0; i <= m_currentSolution.size(); i++){
	  SOLUTION c = m_currentSolution;
	  insert(c, t, i);
	  update(s, delta, c, t);
	}
      }
    }

    // Swapping is always an option for improvin things
    for(unsigned int i=0; i< m_currentSolution.size(); i++){
      for(unsigned int j=i+1; j<m_currentSolution.size(); j++){
	if(i != j){
	  SOLUTION c = m_currentSolution;
	  swap(c, i, j);
	  update(s, delta, c, TokenId::noId());
	}
      }
    }

    // Try removals, assuming it is infeasible
    if(!feasible){
      for(SOLUTION::const_iterator it = m_currentSolution.begin(); it != m_currentSolution.end(); ++it){
	TokenId t = *it;

	if(t->isActive())
	  continue;

	SOLUTION c = m_currentSolution;
	remove(c, t);
	update(s, delta, c, t);
      }
    }
  }

  void GoalManager::insert(SOLUTION& s, const TokenId& t, unsigned int pos){
    checkError(pos <= s.size(), pos << " > " << m_currentSolution.size());

    debugMsg("GoalManager:insert", "Inserting " << t->toString() << " at [" << pos << "] in " << toString(s));

    SOLUTION::iterator it = s.begin();
    for(unsigned int i=0;i<=pos;i++){
      // If we have the insertion point, do it and quit
      if (i == pos){
	s.insert(it, t);
	break;
      }

      // Otherwise advance
      ++it;
    }
  }

  void GoalManager::swap(SOLUTION& s, unsigned int a, unsigned int b){
    debugMsg("GoalManager:swap", "Swapping [" << a << "] and [" << b << "] in " << toString(s));
    SOLUTION::iterator it = s.begin();
    SOLUTION::iterator it_a = s.begin();
    SOLUTION::iterator it_b = s.begin();
    TokenId t_a, t_b;
    unsigned int i = 0;
    unsigned int max_i = std::max(a, b);

    // Compute swap data
    while (i <= max_i){
      TokenId t = *it;
      if (i == a){
	it_a = it;
	t_a = t;
      }
      else if(i == b){
	it_b = it;
	t_b = t;
      }

      i++;
      ++it;
    }

    checkError(a != b && it_a != it_b && t_a != t_b, "Should be different");

    // Execute swap
    it_a = s.erase(it_a);
    s.insert(it_a, t_b);
    it_b = s.erase(it_b);
    s.insert(it_b, t_a);
  }

  void GoalManager::remove(SOLUTION& s, const TokenId& t){
    debugMsg("GoalManager:remove", "Removing " << t->toString() << " from " << toString(s));
    SOLUTION::iterator it = s.begin();
    while((*it) != t && it != s.end()) ++it;
    checkError(it != s.end(), "Not found");
    s.erase(it);
  }

  /**
   * @brief Will promote a solution of equal or better score. This allows a move in a plateau
   */
  void GoalManager::update(SOLUTION& s, TokenId& delta, const SOLUTION& c, TokenId t){
    debugMsg("GoalManager:update", "Evaluating " << toString(c));
    if(compare(c, s) >= 0){
      s = c;
      delta = t;
      debugMsg("GoalManager:update", "Promote " << toString(c));
    }
  }

  int GoalManager::getPriority(const TokenId& token){
    // Slaves take top priority. Cannot be rejected.
    if(token->getMaster().isId())
      return 0;

    // Goals with no priority specified are assumed to be priority 0
    ConstrainedVariableId p = token->getVariable(PRIORITY());
    if(p.isNoId())
      return 0;

    // Now we get the actual priority value
    checkError(p->lastDomain().isSingleton(), p->toString() << " must be set for " << token->toString());

    return (int) p->lastDomain().getSingletonValue();
  }

  void GoalManager::setInitialConditions(){
    // Start time
    const IntervalIntDomain& horizon = DeliberationFilter::getHorizon();
    m_startTime = (int) horizon.getLowerBound();
    m_timeBudget = (int) (horizon.getUpperBound() - horizon.getLowerBound());

    // Need to get the position value of the token that is spanning the current tick.
  }


  /**
   * @brief is s1 better than s2
   * @return WORSE if s1 < s2. EQUAL if s1 == s2. BETTER if s1 > s2
   */
  int GoalManager::compare(const SOLUTION& s1, const SOLUTION& s2){
    bool f1, f2;
    double c1, c2, u1, u2;
    f1 = evaluate(s1, c1, u1);
    f2 = evaluate(s2, c2, u2);

    // Feasibility is dominant.
    if(f1 && !f2)
       return BETTER;
    if(f2 && !f1)
      return WORSE;

    // If both infeasible, cost dominates
    if(!f1 && !f2 && c1 < c2)
	return BETTER;
    if(!f1 && !f2 && c1 > c2)
	return WORSE;

    // If both feasible, utility dominates
    if(f1 && f2 && u1 > u2)
      return BETTER;
    if(f1 && f2 && u1 < u2)
      return WORSE;

    // Finally, a straight comparison
    if((u1/c1) > (u2/c2))
      return BETTER;
    if((u1/c1) < (u2/c2))
      return WORSE;
    else
      return EQUAL;
  }

  Position GoalManager::getPosition(const TokenId& token){
    // Retrieve Variables by parameter name
    ConstrainedVariableId x = token->getVariable(X());
    ConstrainedVariableId y = token->getVariable(Y());

    // Validate token structure for valid position data
    checkError(x.isId(), "No variable for X");
    checkError(y.isId(), "No variable for Y");
    checkError(x->lastDomain().isSingleton(), x->lastDomain().toString());
    checkError(y->lastDomain().isSingleton(), y->lastDomain().toString());

    Position p;
    p.x = x->lastDomain().getSingletonValue();
    p.y = y->lastDomain().getSingletonValue();
    return p;
  }

  /**
   * @brief We assume the curent position is a token on a given timeline that contains the current tick, and has x and y as arguments
   * indicating position.
   */
  Position GoalManager::getCurrentPosition() const {
    static Position sl_pos;
    sl_pos.x = 0.0;
    sl_pos.y = 0.0;

    TICK tick = Agent::instance()->getCurrentTick();

    if(m_positionSource.isId()){
      const std::list<TokenId>& tokens = m_positionSource->getTokenSequence();
      for(std::list<TokenId>::const_iterator it = tokens.begin(); it != tokens.end(); ++it){
	TokenId token = *it;
	if(token->getStart()->lastDomain().getUpperBound() <= tick && token->getEnd()->lastDomain().getLowerBound() > tick)
	  return getPosition(token);

	// Break if past the current time
	if(token->getStart()->lastDomain().getLowerBound() > tick)
	  break;
      }
    }
    return sl_pos;
  }

  /**
   * @todo Implement an accessor for speed to compute time bounds. Should be a parameter, ultimately, of the goal
   * but we can assume a nominal speed.
   */
  double GoalManager::getSpeed() const {
    return 1.0;
  }

  CostEstimator::CostEstimator(const TiXmlElement& configData) {
  }
  CostEstimator::~CostEstimator() {
  }

  EuclideanCostEstimator::EuclideanCostEstimator(const TiXmlElement& configData) :
    CostEstimator(configData) {
  }
  EuclideanCostEstimator::~EuclideanCostEstimator() {
  }
  double EuclideanCostEstimator::computeDistance(const Position& p1, const Position& p2) {
    double result = sqrt(pow(p1.x-p2.x, 2) + pow(p1.y-p2.y, 2));
    debugMsg("GoalManager:computeDistance", "Distance between (" << p1.x << ", " << p1.y << ") => (" << p2.x << ", " << p2.y << ") == " << result);
    return result;
  }
}
