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

    while(choice > 0) {
      cout<<"current state: "<<curr<<" g = "<<g<<"\nactions:\n";

      SuccessorIterator it = env.GetSuccessors(curr.GetStateID(), g);

      if(it.Done()) {
        cout<<"  no actions!\n";
        break;
      }

      it.Next();

      vector<ActionID> actions;

      while(!it.Done()) {
        cout<<"  "<<it.Front().actionID<<": "<<it.Front().stateID<<" cost = "<<(it.Front().g - g)<<endl;
        actions.push_back(it.Front().actionID);
        it.Next();
      }

      choice = -1;
    }
  }

  return 0;
}
