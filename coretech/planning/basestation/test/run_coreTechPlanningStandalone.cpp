#include <iostream>
#include <fstream>

#include "anki/planning/basestation/xythetaPlanner.h"
#include "anki/planning/basestation/xythetaEnvironment.h"

using namespace Anki::Planning;
using namespace std;

int main(int argc, char *argv[])
{
  Anki::Planning::xythetaPlanner planner;

  planner.SayHello();

  if(argc == 3) {

    cout<<"doing some env stuff!\n";

    ofstream pathFile("path.txt");

    xythetaEnvironment env(argv[1], argv[2]);
    xythetaPlan plan;

    int choice = 0;

    State curr = plan.start_;
    Cost g = 0.0;

    while(choice >= 0) {
      cout<<"current state: "<<curr<<" g = "<<g<<"\nactions:\n";

      SuccessorIterator it = env.GetSuccessors(curr.GetStateID(), g);

      if(it.Done()) {
        cout<<"  no actions!\n";
        break;
      }

      it.Next();

      vector<ActionID> actions;
      vector<StateID> results;

      while(!it.Done()) {
        cout<<"  "<<actions.size()<<": (id="<<(int)it.Front().actionID<<") "
            <<State(it.Front().stateID)<<" cost = "<<(it.Front().g - g)<<endl;
        actions.push_back(it.Front().actionID);
        results.push_back(it.Front().stateID);
        it.Next();
      }

      cout<<"> ";
      cin>>choice;

      plan.Push(actions[choice]);
      curr = State(results[choice]);
    }
  }

  return 0;
}
