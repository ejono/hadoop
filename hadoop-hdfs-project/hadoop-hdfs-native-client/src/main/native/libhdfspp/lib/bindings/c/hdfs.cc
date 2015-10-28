/**
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "hdfs_cpp.h"

#include <cstring>
#include <iostream>

using namespace hdfs;

/* Seperate the handles used by the C api from the C++ API*/
struct hdfs_internal {
  hdfs_internal(HadoopFileSystem *p) : filesystem_(p) {}
  hdfs_internal(std::unique_ptr<HadoopFileSystem> p)
      : filesystem_(std::move(p)) {}
  virtual ~hdfs_internal(){};
  HadoopFileSystem *get_impl() { return filesystem_.get(); }
  const HadoopFileSystem *get_impl() const { return filesystem_.get(); }

 private:
  std::unique_ptr<HadoopFileSystem> filesystem_;
};

struct hdfsFile_internal {
  hdfsFile_internal(FileHandle *p) : file_(p) {}
  hdfsFile_internal(std::unique_ptr<FileHandle> p) : file_(std::move(p)) {}
  virtual ~hdfsFile_internal(){};
  FileHandle *get_impl() { return file_.get(); }
  const FileHandle *get_impl() const { return file_.get(); }

 private:
  std::unique_ptr<FileHandle> file_;
};

/* Error handling with optional debug to stderr */
static void ReportError(int errnum, std::string msg) {
  errno = errnum;
#ifdef LIBHDFSPP_C_API_ENABLE_DEBUG
  std::cerr << "Error: errno=" << strerror(errnum) << " message=\"" << msg
            << "\"" << std::endl;
#else
  (void)msg;
#endif
}

/**
 * C API implementations
 **/

int hdfsFileIsOpenForRead(hdfsFile file) {
  /* files can only be open for reads at the moment, do a quick check */
  if (file) {
    return file->get_impl()->IsOpenForRead();
  }
  return false;
}

hdfsFS hdfsConnect(const char *nn, tPort port) {
  HadoopFileSystem *fs = new HadoopFileSystem();
  Status stat = fs->Connect(nn, port);
  if (!stat.ok()) {
    ReportError(ENODEV, "Unable to connect to NameNode.");
    delete fs;
    return nullptr;
  }
  return new hdfs_internal(fs);
}

int hdfsDisconnect(hdfsFS fs) {
  if (!fs) {
    ReportError(ENODEV, "Cannot disconnect null FS handle.");
    return -1;
  }

  delete fs;
  return 0;
}

hdfsFile hdfsOpenFile(hdfsFS fs, const char *path, int flags, int bufferSize,
                      short replication, tSize blocksize) {
  (void)flags;
  (void)bufferSize;
  (void)replication;
  (void)blocksize;
  if (!fs) {
    ReportError(ENODEV, "Cannot perform FS operations with null FS handle.");
    return nullptr;
  }
  FileHandle *f = nullptr;
  Status stat = fs->get_impl()->OpenFileForRead(path, &f);
  if (!stat.ok()) {
    return nullptr;
  }
  return new hdfsFile_internal(f);
}

int hdfsCloseFile(hdfsFS fs, hdfsFile file) {
  if (!fs) {
    ReportError(ENODEV, "Cannot perform FS operations with null FS handle.");
    return -1;
  }
  if (!file) {
    ReportError(EBADF, "Cannot perform FS operations with null File handle.");
    return -1;
  }
  delete file;
  return 0;
}

tSize hdfsPread(hdfsFS fs, hdfsFile file, tOffset position, void *buffer,
                tSize length) {
  if (!fs) {
    ReportError(ENODEV, "Cannot perform FS operations with null FS handle.");
    return -1;
  }
  if (!file) {
    ReportError(EBADF, "Cannot perform FS operations with null File handle.");
    return -1;
  }

  return file->get_impl()->Pread(buffer, length, position);
}
