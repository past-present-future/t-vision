#include <cstdio>
#include <ctime>
#include "include/main.hpp"
#include <dirent.h>
#include <string.h>


int main(int argc, const char *argv[]) {
  struct main_params params;
  char program;
  if (argc > 1){
    for (size_t i = 1; i < argc; ++i){
      printf("%zu: %s\n", i, argv[i]);
      if (argv[i][0] == '-')
      switch(argv[i][1])
      {
      case 'v':
        i++;
        params.device_path=argv[i];
	//demo(&params);
	break;
      case 'd':
	printf("Case d got\n");        
        char *y_loc;
        char dim_str[16];
	i++;
        strcpy(dim_str, argv[i]);
        y_loc = strchr(dim_str, 'x');
        if (y_loc == nullptr)
	{
          printf("Got invalid dims. Exitting...\n");
	  goto end;          
        }
        y_loc[0] = '\0';
	y_loc += 1;
        params.dims.x = atoi(dim_str);
        params.dims.y = atoi(y_loc);
	printf("Dims set to: ( %zu , %zu )\n", params.dims.x, params.dims.y);
	break;
      case 'p':
        i++;
	program=argv[i][0];
        //shader_playground(&params);
        break;
      case 's':
	i++;
        strcpy(params.frag_path, argv[i]);
	printf("Fragment shader path: %s", params.frag_path);        
	break;        
      default:printf("Unknown option selected\nLooking for video device");
      }
       //shader_playground(&params);
    }
  }
  else {
    DIR *dev_dir, *v4l2_dev_dir;
    struct dirent *dir, *v4l2_dir;
    char path[64];
    dev_dir = opendir("/dev");
    v4l2_dev_dir = opendir("/sys/class/video4linux");
    if (dev_dir) {

      while ((v4l2_dir = readdir(v4l2_dev_dir)) != nullptr) {
        if (v4l2_dir->d_name[0] == '.')
	  continue;
	printf("Searching for %s\n", v4l2_dir->d_name);
        while ((dir = readdir(dev_dir)) != nullptr) {
	  if (!strcmp(dir->d_name, v4l2_dir->d_name)){
            printf("Found %s\n", dir->d_name);
            sprintf(path, "/dev/%s", dir->d_name);
	    break;
	  }
        }
      }
      
    }
      demo(&params);
  }
  switch (program) {
  case 'd':
    demo(&params);
    break;
  case 'a':
    render_api_test(&params);
    break;
  case 's':
    shader_playground(&params);
    break;
  }
    
end:
  return 0;
}
