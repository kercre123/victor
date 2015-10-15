/**
 * File: run_coreTechPlanningStandalone.cpp
 *
 * Author: Brad Neuman
 * Created: 2015-09-23
 *
 * Description: Standalone executable for running the planner
 *
 * Copyright: Anki, Inc. 2015
 *
 **/

#include "anki/planning/basestation/xythetaEnvironment.h"
#include "anki/planning/basestation/xythetaPlanner.h"
#include "anki/planning/basestation/xythetaPlannerContext.h"
#include "json/json.h"
#include "util/logging/logging.h"
#include "util/logging/printfLoggerProvider.h"
#include <fstream>
#include <getopt.h>
#include <iomanip>
#include <iostream>
#include <map>

using namespace Anki::Planning;
using namespace std;

Anki::Util::PrintfLoggerProvider* loggerProvider = nullptr;

void writePath(string filename, const xythetaEnvironment& env, const xythetaPlan& plan)
{
  ofstream outfile(filename);
  vector<State_c> plan_c;
  env.ConvertToXYPlan(plan, plan_c);
  for(size_t i=0; i<plan_c.size(); ++i)
    outfile<<plan_c[i].x_mm<<' '<<plan_c[i].y_mm<<' '<<plan_c[i].theta<<endl;

  outfile.close();
}

void usage()
{
  cout<<"usage: ctiPlanningStandalone -p mprim.json [--manual | --context env.json]\n";
}

int main(int argc, char *argv[])
{
  cout<<"Welcome to xythetaPlanner.\n";

  loggerProvider = new Anki::Util::PrintfLoggerProvider();
  loggerProvider->SetMinLogLevel(0);
  Anki::Util::gLoggerProvider = loggerProvider;

  bool gotMprim = false;
  xythetaPlannerContext context;

  int doManualPlanner = 0;
  int doContextPlan = 0;
  Json::Value contextJson;

  static struct option opts[] = {
    {"help", no_argument, 0, 'h'},
    {"mprim", required_argument, 0, 'p'},
    {"manual", no_argument, &doManualPlanner, 1},
    {"context", required_argument, 0, 'c'},
    {0, 0, 0, 0}
  };

  int option_index = 0;

  while(true) {
    int c = getopt_long( argc, argv, "hp:c:", opts, &option_index );
    if (c == -1) {
      break;
    }
    switch(c) {
      case 0:
        break;

      case 'h':
        usage();
        return 0;

      case 'p':
        gotMprim = context.env.ReadMotionPrimitives(optarg);
        if( ! gotMprim ) {
          cerr<<"Could not read motion primitives from file "<<optarg<<endl;
          return -1;
        }
        break;

      case 'c': {
        doContextPlan = 1;
        
        Json::Reader jsonReader;

        ifstream contextStream(optarg);
        bool success = jsonReader.parse(contextStream, contextJson);
        if( ! success ) {
          cerr<<"Could not read context from file "<<optarg<<endl;
          return -1;
        }
        break;
      }

      default:
        usage();
        return -1;
    }
  }

  if( ! gotMprim ) {
    cerr<<"Must pass in motion primitives!\n";
    usage();
    return -1;
  }


  if( doContextPlan ) {
    bool success = context.Import(contextJson);
    if( ! success ) {
      cerr<<"Could not import planner context\n";
      return -1;
    }

    xythetaPlanner planner(context);
    planner.Replan();

    printf("complete. Path: \n");
    context.env.PrintPlan(planner.GetPlan());

    writePath("path.txt", context.env, planner.GetPlan());
    cout<<"check path.txt\n";
  }

  if( doManualPlanner ) {
    xythetaPlan plan;

    int choice = 0;

    State curr = plan.start_;
    Cost g = 0.0;

    while(choice >= 0) {
      cout<<"current state: "<<curr<<" g = "<<g<<"\nactions:\n";

      SuccessorIterator it = context.env.GetSuccessors(curr.GetStateID(), g);

      if(it.Done(context.env)) {
        cout<<"  no actions!\n";
        break;
      }

      it.Next(context.env);

      std::map<ActionID, StateID> results;

      while(!it.Done(context.env)) {
        MotionPrimitive prim;
        string name;
        if(!context.env.GetMotion(curr.theta, it.Front().actionID, prim)) {
          printf("error: internal error! Could not get primitive id %d at theta %d!\n",
                     it.Front().actionID,
                     curr.theta);
        }
        name = context.env.GetActionType(it.Front().actionID).GetName();

        cout<<"  "<<(int)it.Front().actionID<<": "<<left<<setw(22)<<name
            <<State(it.Front().stateID)<<" cost = "
            <<(it.Front().g - g)<<endl;
        results[it.Front().actionID] = it.Front().stateID;
        it.Next(context.env);
      }

      cout<<" -1: exit\n> ";
      cin>>choice;

      if(choice >= 0 && results.count(choice) > 0) {
        plan.Push(choice, 0.0);
        writePath("path.txt", context.env, plan);

        curr = State(results[choice]);
      }
    }

    printf("complete. Path: \n");
    context.env.PrintPlan(plan);

    cout<<"final state: "<<context.env.State2State_c(context.env.GetPlanFinalState(plan))<<endl;

    printf("\n\nTestPlan:\n");
    for(auto action : plan.actions_) {
      printf("plan.Push(%d);\n", action);
    }
  }

  return 0;
}
