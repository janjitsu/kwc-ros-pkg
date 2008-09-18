#include "ROSControllerAdapter.hh"
#include "IntervalDomain.hh"
#include "Token.hh"
#include <pr2_msgs/MoveArmState.h>
#include <pr2_msgs/MoveArmGoal.h>

namespace TREX {

  class ArmControllerAdapter: public ROSControllerAdapter<pr2_msgs::MoveArmState, pr2_msgs::MoveArmGoal> {
  public:

    ArmControllerAdapter(const LabelStr& agentName, const TiXmlElement& configData)
      : ROSControllerAdapter<pr2_msgs::MoveArmState, pr2_msgs::MoveArmGoal>(agentName, configData){
    }

    virtual ~ArmControllerAdapter(){}

  protected:

    bool readJointIndex(const std::string& jointName, unsigned int& ind){
      for(unsigned int i = 0; i < rosNames().size(); i++){
	if(rosNames()[i] == jointName){
	  ind = i;
	  return true;
	}
      }

      return false;
    }
    
    void fillActiveObservationParameters(ObservationByValue* obs){
      for(unsigned int i = 0; i < stateMsg.get_goal_size(); i++){
	unsigned int ind;
	if(readJointIndex(stateMsg.goal[i].name, ind))
	   obs->push_back(nddlNames()[ind], new IntervalDomain(stateMsg.goal[i].position));
      }
    }

    void fillInactiveObservationParameters(ObservationByValue* obs){  
      for(unsigned int i = 0; i < stateMsg.get_configuration_size(); i++){
	unsigned int ind;
	if(readJointIndex(stateMsg.configuration[i].name, ind))
	   obs->push_back(nddlNames()[ind], new IntervalDomain(stateMsg.configuration[i].position));
      }
    }

    void fillRequestParameters(pr2_msgs::MoveArmGoal& goalMsg, const TokenId& goalToken){
      goalMsg.set_configuration_size(nddlNames().size());
      for(unsigned int i = 0; i<nddlNames().size(); i++){
	const IntervalDomain& dom = goalToken->getVariable(nddlNames()[i])->lastDomain();
	assertTrue(dom.isSingleton(), "Values for dispatch are not bound");
	goalMsg.configuration[i].name = rosNames()[i];
	goalMsg.configuration[i].position = dom.getSingletonValue();
      }
    }
  };  

  // Allocate Factory
  TeleoReactor::ConcreteFactory<ArmControllerAdapter> ArmControllerAdapter_Factory("ArmControllerAdapter");
}
