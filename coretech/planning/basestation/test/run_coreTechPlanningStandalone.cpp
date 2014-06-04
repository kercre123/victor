#include <iostream>
#include <iomanip>
#include <fstream>

#include "anki/planning/basestation/xythetaPlanner.h"
#include "anki/planning/basestation/xythetaEnvironment.h"
#include <map>

using namespace Anki::Planning;
using namespace std;

void writePath(string filename, const xythetaEnvironment& env, const xythetaPlan& plan)
{
  ofstream outfile(filename);
  vector<State_c> plan_c;
  env.ConvertToXYPlan(plan, plan_c);
  for(size_t i=0; i<plan_c.size(); ++i)
    outfile<<plan_c[i].x_mm<<' '<<plan_c[i].y_mm<<' '<<plan_c[i].theta<<endl;

  outfile.close();
}

int main(int argc, char *argv[])
{
  cout<<"Welcome to xythetaPlanner. argc = "<<argc<<endl;
  if(argc == 3) {
    cout<<"doing some env stuff!\n";

    xythetaEnvironment env;
    if(!env.Init(argv[1], argv[2])) {
      return -1;
    }
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

      std::map<ActionID, StateID> results;

      while(!it.Done()) {
        MotionPrimitive prim;
        string name;
        if(!env.GetMotion(curr.theta, it.Front().actionID, prim)) {
          printf("error: internal error! Could not get primitive id %d at theta %d!\n",
                     it.Front().actionID,
                     curr.theta);
        }
        name = env.GetActionType(it.Front().actionID).GetName();

        cout<<"  "<<(int)it.Front().actionID<<": "<<left<<setw(22)<<name
            <<State(it.Front().stateID)<<" cost = "
            <<(it.Front().g - g)<<endl;
        results[it.Front().actionID] = it.Front().stateID;
        it.Next();
      }

      cout<<"> ";
      cin>>choice;

      if(choice >= 0 && results.count(choice) > 0) {
        plan.Push(choice);
        writePath("path.txt", env, plan);

        curr = State(results[choice]);
      }
    }
  }
  else if(argc == 7 || argc == 6) {
    cout<<"running planner!\n";

    xythetaEnvironment env;
    if(!env.Init(argv[1], argv[2])) {
      return -1;
    }
    xythetaPlanner planner(env);
    float theta = 0.0;
    if(argc == 7) {
      planner.AllowFreeTurnInPlaceAtGoal();
    }

    State_c goal(atof(argv[3]), atof(argv[4]), atof(argv[5]));

    planner.SetGoal(goal);
    planner.ComputePath();

    writePath("path.txt", env, planner.GetPlan());
    cout<<"done! check path.txt\n";
  }
  else if(argc == 9) {
    xythetaEnvironment env;
    if(!env.Init(argv[1], argv[2])) {
      return -1;
    }
    xythetaPlanner planner(env);
    float theta = 0.0;

    State_c goal(atof(argv[3]), atof(argv[4]), atof(argv[5]));
    State_c start(atof(argv[6]), atof(argv[7]), atof(argv[8]));

    planner.SetGoal(goal);
    planner.SetStart(start);
    // planner.AllowFreeTurnInPlaceAtGoal();

    planner.ComputePath();

    writePath("path.txt", env, planner.GetPlan());
    cout<<"done! check path.txt\n";
  }

  return 0;
}
