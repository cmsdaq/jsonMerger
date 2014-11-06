
#include "interface/JSONSerializer.h"
#include "interface/FileIO.h"
#include "interface/DataPointCollection.h"
#include <iostream>
#include <sys/utsname.h>


using namespace jsoncollector;

int  main(int argc, char**argv)
{
  if (argc<3) {
    std::cout << "Not enough parameters" << std::endl;
    return 1;
  }

  struct utsname unameData;
  uname(&unameData);
  std::string host_=unameData.nodename;



  std::map<std::string,DataPointDefinition*> defMap_;
  DataPointCollection target(&host_);
  std::vector<DataPointCollection*> sources;

  char cwd[4096];
  if (getcwd(cwd, sizeof(cwd)) == NULL) {
    std::cout << "Could not get current working directory" << std::endl;
    return 2;
  }
  for (int i=2;i<argc;i++) {
    std::string name;
    if (argv[i][0]!='/') {
      name = std::string(cwd)+'/'+argv[i];
    }
    else
      name = argv[i];
    sources.push_back(new DataPointCollection(name,&defMap_));
    if (!sources.back()->isInitialized()) {
      std::cout << "Could not load file "<< name << std::endl;
      return 3;
    }
  }
  if (!target.mergeCollections(sources)) {
      std::cout << "Could not merge data " << std::endl;
      return 4;
  }

  Json::Value serializeRoot = target.mergeAndSerialize();

  Json::StyledWriter writer;
  std::string && result = writer.write(serializeRoot);
  //stat argv[1]..
  std::string outName;
  if (argv[1][0]!='/') outName=std::string(cwd)+'/'+argv[1];
  else outName=argv[1];
  bool written = FileIO::writeStringToFile(outName, result);
  if (!written) {
    std::cout << "Could not write file " << outName << std::endl;
    return 5;
  }
}
