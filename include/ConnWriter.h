#pragma once

#include <ios>
#include <list>
#include <memory>

class WriteObject {
public:
  std::streamsize start;
  std::streamsize len;
  std::shared_ptr<char> buff;
};

class ConnWriter {
public:
  int fd;

private:
  bool isClosing = false;
  std::list<std::shared_ptr<WriteObject>> objs;

public:
  void EntireWrite(std::shared_ptr<char> buff, std::streamsize len);
  void LingerClose();
  void OnWriteable();

private:
  void EntireWriteWhenEmpty(std::shared_ptr<char> buff, std::streamsize len);
  void EntireWriteWhenNotEmpty(std::shared_ptr<char> buff, std::streamsize len);
  bool WriteFrontObj();
};
