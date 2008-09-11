/*********************************************************************
 * Software License Agreement (BSD License)
 *
 *  Copyright (c) 2008, Willow Garage, Inc.
 *  All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions
 *  are met:
 *
 *   * Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above
 *     copyright notice, this list of conditions and the following
 *     disclaimer in the documentation and/or other materials provided
 *     with the distribution.
 *   * Neither the name of the Willow Garage nor the names of its
 *     contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 *  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 *  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 *  FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 *  COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 *  INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 *  BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 *  LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 *  CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 *  LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 *  ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 *  POSSIBILITY OF SUCH DAMAGE.
 *********************************************************************/

#include <stdio.h>
#include <getopt.h>
#include <execinfo.h>
#include <signal.h>
#include <sys/mman.h>

#include <mechanism_control/mechanism_control.h>
#include <ethercat_hardware/ethercat_hardware.h>

#include <ros/node.h>
#include <std_srvs/Empty.h>

static struct
{
  char *program_;
  char *interface_;
  char *xml_;
  bool allow_override_;
} g_options;

void Usage(string msg = "")
{
  fprintf(stderr, "Usage: %s [options]\n", g_options.program_);
  if (msg != "")
  {
    fprintf(stderr, "Error: %s\n", msg.c_str());
    exit(-1);
  }
  else
  {
    exit(0);
  }
}

static int quit = 0;
static const int NSEC_PER_SEC = 1e+9;

void *controlLoop(void *)
{
  // Initialize the hardware interface
  EthercatHardware ec;
  ec.init(g_options.interface_);

  // Create mechanism control
  MechanismControl mc(ec.hw_);
  MechanismControlNode mcn(&mc);

  // Load robot description
  TiXmlDocument xml(g_options.xml_);
  xml.LoadFile();
  TiXmlElement *root = xml.FirstChildElement("robot");
  if (!root)
  {
    fprintf(stderr, "Could not load the xml file: %s\n", g_options.xml_);
    exit(1);
  }

  // Register actuators with mechanism control
  ec.initXml(root, g_options.allow_override_);

  // Initialize mechanism control from robot description
  mcn.initXml(root);

  // Spawn controllers
  // TODO what file does this come from?
  // Can this be pushed down to mechanism control?
  for (TiXmlElement *elt = root->FirstChildElement("controller"); elt ; elt = elt->NextSiblingElement("controller"))
  {
    mc.spawnController(elt->Attribute("type"), elt->Attribute("name"), elt);
  }

  // Switch to hard real-time
#if defined(__XENO__)
  pthread_set_mode_np(0, PTHREAD_PRIMARY|PTHREAD_WARNSW);
#endif

  struct timespec tick;
  clock_gettime(CLOCK_REALTIME, &tick);
  int period = 1e+6; // 1 ms in nanoseconds

  while (!quit)
  {
    ec.update();
    mcn.update();

    tick.tv_nsec += period;
    while (tick.tv_nsec >= NSEC_PER_SEC)
    {
      tick.tv_nsec -= NSEC_PER_SEC;
      tick.tv_sec++;
    }
    clock_nanosleep(CLOCK_REALTIME, TIMER_ABSTIME, &tick, NULL);
  }

  /* Shutdown all of the motors on exit */
  for (unsigned int i = 0; i < ec.hw_->actuators_.size(); ++i)
  {
    ec.hw_->actuators_[i]->command_.enable_ = false;
    ec.hw_->actuators_[i]->command_.effort_ = 0;
  }
  ec.update();

  // Switch back from hard real-time
#if defined(__XENO__)
  pthread_set_mode_np(PTHREAD_PRIMARY, 0);
#endif

  return 0;
}

void quitRequested(int sig)
{
  quit = 1;
}

class Shutdown {
public:
  bool shutdownService(std_srvs::Empty::request &req, std_srvs::Empty::response &resp)
  {
    quitRequested(0);
    return true;
  }
};

void warnOnSecondary(int sig)
{
  void *bt[32];
  int nentries;

  // Dump a backtrace of the frame which caused the switch to
  // secondary mode
  nentries = backtrace(bt, sizeof(bt) / sizeof(bt[0]));
  backtrace_symbols_fd(bt, nentries, fileno(stdout));
  printf("\n");
  fflush(stdout);
}

static pthread_t rtThread;
static pthread_attr_t rtThreadAttr;

int main(int argc, char *argv[])
{
  // Keep the kernel from swapping us out
  mlockall(MCL_CURRENT | MCL_FUTURE);

  // Initialize ROS and parse command-line arguments
  ros::init(argc, argv);

  // Parse options
  g_options.program_ = argv[0];
  while (1)
  {
    static struct option long_options[] = {
      {"help", no_argument, 0, 'h'},
      {"allow_override", no_argument, 0, 'a'},
      {"interface", required_argument, 0, 'i'},
      {"xml", required_argument, 0, 'x'},
    };
    int option_index = 0;
    int c = getopt_long(argc, argv, "ahi:x:", long_options, &option_index);
    if (c == -1) break;
    switch (c)
    {
      case 'h':
        Usage();
        break;
      case 'a':
        g_options.allow_override_ = 1;
        break;
      case 'i':
        g_options.interface_ = optarg;
        break;
      case 'x':
        g_options.xml_ = optarg;
        break;
    }
  }
  if (optind < argc)
  {
    Usage("Extra arguments");
  }

  if (!g_options.interface_)
    Usage("You must specify a network interface");
  if (!g_options.xml_)
    Usage("You must specify a robot description XML file");

  ros::node *node = new ros::node("mechanism_control",
                                  ros::node::DONT_HANDLE_SIGINT);

  // Catch attempts to quit
  signal(SIGTERM, quitRequested);
  signal(SIGINT, quitRequested);
  signal(SIGHUP, quitRequested);

  // Catch if we fall back to secondary mode
  signal(SIGXCPU, warnOnSecondary);

  // Set up thread scheduler for realtime
  pthread_attr_init(&rtThreadAttr);
  pthread_attr_setinheritsched(&rtThreadAttr, PTHREAD_EXPLICIT_SCHED);
  pthread_attr_setschedpolicy(&rtThreadAttr, SCHED_FIFO);

  node->advertise_service("shutdown", &Shutdown::shutdownService);

  //Start thread
  int rv;
  if ((rv = pthread_create(&rtThread, &rtThreadAttr, controlLoop, 0)) != 0)
  {
    node->log(ros::FATAL, "Unable to create realtime thread: rv = %d\n", rv);
  }

  pthread_join(rtThread, 0);

  ros::fini();
  delete node;

  return 0;
}
