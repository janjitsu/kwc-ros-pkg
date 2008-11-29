#include <map>
#include <list>
#include <string>
#include <stdexcept>
#include <string.h>
#include "ros/common.h"
#include "ros/time.h"
#include "borg.h"
#include "cam_dc1394.h"

using namespace borg;
using std::string;
using std::map;
using std::list;

Borg::Borg(uint32_t opts)
{
  string cfg_path = ros::get_package_path("borg");
  cfg_path += "/borg-config";
  FILE *f = fopen(cfg_path.c_str(), "r");
  if (!f)
  {
    printf("Unable to open the borg-config file in the borg package. Please "
           "create a symlink from one of the borg configuration files in "
           "borg/config, like this:\n"
           "  roscd borg\n"
           "  ln -s config/avt_pike.config borg-config\n");
    exit(1);
  }
  string cam_str;
  map<string, uint32_t> cam_settings;
  for (int line = 1; !feof(f); line++)
  {
    char linebuf[200];
    char key[50], value[50];
    if (!fgets(linebuf, sizeof(linebuf), f))
      continue;
    if (linebuf[0] == '#')
      continue;
    int n = sscanf(linebuf, "%50s %50s\n", key, value);
    if (n == 0)
      continue;
    if (n != 2)
    {
      printf("unable to parse line %d of the borg-config file\n", line);
      throw std::runtime_error("borg init failed");
    }
    if (!strcmp(key, "fps"))
      fps = atoi(value);
    else if (!strcmp(key, "cam"))
      cam_str = string(value);
    else if (!strncmp(key, "cam_", 4))
      cam_settings[string(key+4)] = atoi(value);
    else
      printf("unknown key = [%s] with value = [%s]\n", key, value);
  }
  fclose(f);
  if (opts & INIT_CAM)
  {
    if (cam_str == string("dc1394"))
    {
      printf("calling dc1394 init\n");
      cam = new CamDC1394();
      if (!cam->init())
        throw std::runtime_error("unable to init camera.\n");
      for (map<string,uint32_t>::iterator s = cam_settings.begin();
           s != cam_settings.end(); ++s)
        cam->set(s->first.c_str(), s->second);
    }
    else
    {
      printf("unknown camera\n");
      throw std::runtime_error("borg init failed");
    }
  }
}

Borg::~Borg()
{
  if (cam)
  {
    cam->shutdown();
    delete cam;
  }
}

struct ScanImage
{
  uint8_t *raster;
  double t;
};

bool Borg::scan()
{
  list<ScanImage *> images;
  cam->startImageStream();
  for (int i = 0; i < 10; i++)
  {
    ScanImage *si = new ScanImage;
    si->raster = new uint8_t[640*480];
    si->t = ros::Time::now().to_double();
    if (!cam->savePhoto(si->raster))
    {
      cam->stopImageStream();
      return false;
    }
    images.push_back(si);
  }
  printf("captured %d images\n", images.size());
  cam->stopImageStream();
  // flush to disk
  for (list<ScanImage *>::iterator i = images.begin(); i != images.end(); ++i)
  {
    char fname[100];
    snprintf(fname, sizeof(fname), "img_%.6f.pgm", (*i)->t);
    FILE *f = fopen(fname, "wb");
    if (!f)
      throw std::runtime_error("couldn't open pgm file for output");
    fprintf(f, "P5\n640 480\n255\n");
    fwrite((*i)->raster, 1, 640 * 480, f);
    fclose(f);
    delete[] (*i)->raster;
    delete *i;
  }
  images.clear();
  return true;
}

