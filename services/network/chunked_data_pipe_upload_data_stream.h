// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_NETWORK_CHUNKED_DATA_PIPE_UPLOAD_DATA_STREAM_H_
#define SERVICES_NETWORK_CHUNKED_DATA_PIPE_UPLOAD_DATA_STREAM_H_

#include <stdint.h>

#include <memory>

#include "base/component_export.h"
#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/optional.h"
#include "mojo/public/cpp/system/data_pipe.h"
#include "mojo/public/cpp/system/simple_watcher.h"
#include "net/base/completion_once_callback.h"
#include "net/base/net_errors.h"
#include "net/base/upload_data_stream.h"
#include "services/network/public/cpp/resource_request_body.h"
#include "services/network/public/mojom/chunked_data_pipe_getter.mojom.h"

namespace net {
class IOBuffer;
}

namespace network {

// A subclass of net::UploadDataStream to read data pipes and provide the result
// as a chunked upload.
class COMPONENT_EXPORT(NETWORK_SERVICE) ChunkedDataPipeUploadDataStream
    : public net::UploadDataStream {
 public:
  // |resource_request_body| is just passed in to keep the object around for the
  // life of the UploadDataStream.
  ChunkedDataPipeUploadDataStream(
      scoped_refptr<ResourceRequestBody> resource_request_body,
      mojom::ChunkedDataPipeGetterPtr chunked_data_pipe_getter);

  ~ChunkedDataPipeUploadDataStream() override;

 private:
  // net::UploadDataStream implementation.
  int InitInternal(const net::NetLogWithSource& net_log) override;
  int ReadInternal(net::IOBuffer* buf, int buf_len) override;
  void ResetInternal() override;

  // Callback invoked by ChunkedDataPipeGetter::GetSize. Can be called any time
  // - could be called before any of the body is read, could be called after the
  // entire body is read, could end up not being called, in the case the other
  // end of the pipe is torn down before the entire upload body is received.
  void OnSizeReceived(int32_t status, uint64_t size);

  // Called by |handle_watcher_| when data is available or the pipe was closed,
  // and there's a pending Read() call.
  void OnHandleReadable(MojoResult result);

  void OnDataPipeGetterClosed();

  scoped_refptr<ResourceRequestBody> resource_request_body_;
  mojom::ChunkedDataPipeGetterPtr chunked_data_pipe_getter_;
  mojo::ScopedDataPipeConsumerHandle data_pipe_;
  // Watcher for |data_pipe_|. Only armed while there's a pending read.
  mojo::SimpleWatcher handle_watcher_;

  // Write buffer and its length. Populated when Read() is called but returns
  // ERR_IO_PENDING. Cleared once the read completes, or ResetInternal() is
  // invoked.
  scoped_refptr<net::IOBuffer> buf_;
  int buf_len_ = 0;

  // Total size of input, as passed to ReadCallback(). nullptr until size is
  // received.
  base::Optional<uint64_t> size_;

  uint64_t bytes_read_ = 0;

  // Set to a net::Error other than net::OK if the DataPipeGetter returns an
  // error.
  int status_ = net::OK;

  DISALLOW_COPY_AND_ASSIGN(ChunkedDataPipeUploadDataStream);
};

}  // namespace network

#endif  // SERVICES_NETWORK_CHUNKED_DATA_PIPE_UPLOAD_DATA_STREAM_H_
