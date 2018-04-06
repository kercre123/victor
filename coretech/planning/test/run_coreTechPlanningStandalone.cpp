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

#include "coretech/planning/engine/xythetaEnvironment.h"
#include "coretech/planning/engine/xythetaPlanner.h"
#include "coretech/planning/engine/xythetaPlannerContext.h"
#include "json/json.h"
#include "util/logging/logging.h"
#include "util/logging/printfLoggerProvider.h"
#include <fstream>
#include <getopt.h>
#include <iomanip>
#include <iostream>
#include <libgen.h>
#include <map>

using namespace Anki::Planning;
using namespace std;

Anki::Util::PrintfLoggerProvider* loggerProvider = nullptr;

void writePath(string filename, const xythetaEnvironment& env, const xythetaPlan& plan)
{
  ofstream outfile(filename);
  vector<State_c> plan_c;
  env.GetActionSpace().ConvertToXYPlan(plan, plan_c);
  for(size_t i=0; i<plan_c.size(); ++i)
    outfile<<plan_c[i].x_mm<<' '<<plan_c[i].y_mm<<' '<<plan_c[i].theta<<endl;

  outfile.close();
}

void usage()
{
  cout<<"usage: ctiPlanningStandalone -p mprim.json [--manual | --context env.json] --stat\n";
  cout<<"       --stat causes the program to output a single line suitable for appending to a state file\n";
}

int main(int argc, char *argv[])
{

  bool gotMprim = false;
  bool stats = false;
  bool quiet = false;
  xythetaPlannerContext context;

  int doManualPlanner = 0;
  int doContextPlan = 0;
  Json::Value contextJson;

  std::string contextFilenameBase;

  static struct option opts[] = {
    {"help", no_argument, 0, 'h'},
    {"mprim", required_argument, 0, 'p'},
    {"manual", no_argument, &doManualPlanner, 1},
    {"context", required_argument, 0, 'c'},
    {"stat", no_argument, 0, 's'},
    {0, 0, 0, 0}
  };

  int option_index = 0;

  while(true) {
    int c = getopt_long( argc, argv, "hp:c:s", opts, &option_index );
    if (c == -1) {
      break;
    }
    switch(c) {
      case 0:
        break;

      case 'h':
        usage();
        return 0;

      case 's':
        stats = true;
        quiet = true;
        break;

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

        char* contextFilenameBase_c = basename(optarg);
        if( contextFilenameBase_c != nullptr ) {
          contextFilenameBase = contextFilenameBase_c;
        }

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

  if( ! quiet ) {
    cout<<"Welcome to xythetaPlanner.\n";

    loggerProvider = new Anki::Util::PrintfLoggerProvider();
    loggerProvider->SetMinLogLevel(Anki::Util::ILoggerProvider::LOG_LEVEL_DEBUG);
    Anki::Util::gLoggerProvider = loggerProvider;
  }


  if( doContextPlan ) {
    bool success = context.Import(contextJson);
    if( ! success ) {
      cerr<<"Could not import planner context\n";
      return -1;
    }

    using namespace std::chrono;

    high_resolution_clock::time_point envStartT = high_resolution_clock::now();
    context.env.PrepareForPlanning();
    high_resolution_clock::time_point envEndT = high_resolution_clock::now();

    xythetaPlanner planner(context);
    planner.Replan();

    if( ! quiet ) {
      printf("complete. Path: \n");
      context.env.GetActionSpace().PrintPlan(planner.GetPlan());

      char* cwd=NULL;
      size_t size = 0;
      cwd = getcwd(cwd,size);

      writePath("path.txt", context.env, planner.GetPlan());
      cout<<"check "<<cwd<<"/path.txt\n";
    }

    if( stats ) {
      duration<double> time_d = duration_cast<duration<double>>(envEndT - envStartT);
      double envTime = time_d.count();

      Json::Value statsJson;
      statsJson["context"] = contextFilenameBase;
      statsJson["planTime"] = planner.GetLastPlanTime();
      statsJson["envTime"] = envTime;
      statsJson["expansions"] = planner.GetLastNumExpansions();
      statsJson["considerations"] = planner.GetLastNumConsiderations();
      statsJson["cost"] = planner.GetFinalCost();
      
      Json::StyledStreamWriter writer("  ");
      writer.write(cout, statsJson);
      cout << std::endl;
    }
  }

  if( doManualPlanner ) {
    xythetaPlan plan;

    int choice = 0;

    GraphState curr = plan.start_;
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
        if(!context.env.TryGetRawMotion(curr.theta, it.Front().actionID, prim)) {
          printf("error: internal error! Could not get primitive id %d at theta %d!\n",
                     it.Front().actionID,
                     curr.theta);
        }
        name = context.env.GetActionSpace().GetActionType(it.Front().actionID).GetName();

        cout<<"  "<<(int)it.Front().actionID<<": "<<left<<setw(22)<<name
            <<GraphState(it.Front().stateID)<<" cost = "
            <<(it.Front().g - g)<<endl;
        results[it.Front().actionID] = it.Front().stateID;
        it.Next(context.env);
      }

      cout<<" -1: exit\n> ";
      cin>>choice;

      if(choice >= 0 && results.count(choice) > 0) {
        plan.Push(choice, 0.0);
        writePath("path.txt", context.env, plan);

        curr = GraphState(results[choice]);
      }
    }

    printf("complete. Path: \n");
    context.env.GetActionSpace().PrintPlan(plan);

    cout<<"final state: "<<context.env.State2State_c(context.env.GetActionSpace().GetPlanFinalState(plan))<<endl;

    printf("\n\nTestPlan:\n");
    for(size_t i = 0; i < plan.Size(); i++) 
    {
      printf("plan.Push(%d);\n", plan.GetAction(i));
    }
  }

  return 0;
}
