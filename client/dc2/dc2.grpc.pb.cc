// Generated by the gRPC C++ plugin.
// If you make any local change, they will be lost.
// source: dc2.proto

#include "dc2.pb.h"
#include "dc2.grpc.pb.h"

#include <grpc++/impl/codegen/async_stream.h>
#include <grpc++/impl/codegen/async_unary_call.h>
#include <grpc++/impl/codegen/channel_interface.h>
#include <grpc++/impl/codegen/client_unary_call.h>
#include <grpc++/impl/codegen/method_handler_impl.h>
#include <grpc++/impl/codegen/rpc_service_method.h>
#include <grpc++/impl/codegen/service_type.h>
#include <grpc++/impl/codegen/sync_stream.h>
namespace dc2 {

static const char* ChatHub_method_names[] = {
  "/dc2.ChatHub/Channels",
  "/dc2.ChatHub/ChannelInfo",
  "/dc2.ChatHub/SetChannelOptions",
};

std::unique_ptr< ChatHub::Stub> ChatHub::NewStub(const std::shared_ptr< ::grpc::ChannelInterface>& channel, const ::grpc::StubOptions& options) {
  std::unique_ptr< ChatHub::Stub> stub(new ChatHub::Stub(channel));
  return stub;
}

ChatHub::Stub::Stub(const std::shared_ptr< ::grpc::ChannelInterface>& channel)
  : channel_(channel), rpcmethod_Channels_(ChatHub_method_names[0], ::grpc::RpcMethod::SERVER_STREAMING, channel)
  , rpcmethod_ChannelInfo_(ChatHub_method_names[1], ::grpc::RpcMethod::NORMAL_RPC, channel)
  , rpcmethod_SetChannelOptions_(ChatHub_method_names[2], ::grpc::RpcMethod::NORMAL_RPC, channel)
  {}

::grpc::ClientReader< ::dc2::ChatChannel>* ChatHub::Stub::ChannelsRaw(::grpc::ClientContext* context, const ::dc2::ChannelsReq& request) {
  return new ::grpc::ClientReader< ::dc2::ChatChannel>(channel_.get(), rpcmethod_Channels_, context, request);
}

::grpc::ClientAsyncReader< ::dc2::ChatChannel>* ChatHub::Stub::AsyncChannelsRaw(::grpc::ClientContext* context, const ::dc2::ChannelsReq& request, ::grpc::CompletionQueue* cq, void* tag) {
  return ::grpc::ClientAsyncReader< ::dc2::ChatChannel>::Create(channel_.get(), cq, rpcmethod_Channels_, context, request, true, tag);
}

::grpc::ClientAsyncReader< ::dc2::ChatChannel>* ChatHub::Stub::PrepareAsyncChannelsRaw(::grpc::ClientContext* context, const ::dc2::ChannelsReq& request, ::grpc::CompletionQueue* cq) {
  return ::grpc::ClientAsyncReader< ::dc2::ChatChannel>::Create(channel_.get(), cq, rpcmethod_Channels_, context, request, false, nullptr);
}

::grpc::Status ChatHub::Stub::ChannelInfo(::grpc::ClientContext* context, const ::dc2::ChannelInfoReq& request, ::dc2::ChatChannel* response) {
  return ::grpc::BlockingUnaryCall(channel_.get(), rpcmethod_ChannelInfo_, context, request, response);
}

::grpc::ClientAsyncResponseReader< ::dc2::ChatChannel>* ChatHub::Stub::AsyncChannelInfoRaw(::grpc::ClientContext* context, const ::dc2::ChannelInfoReq& request, ::grpc::CompletionQueue* cq) {
  return ::grpc::ClientAsyncResponseReader< ::dc2::ChatChannel>::Create(channel_.get(), cq, rpcmethod_ChannelInfo_, context, request, true);
}

::grpc::ClientAsyncResponseReader< ::dc2::ChatChannel>* ChatHub::Stub::PrepareAsyncChannelInfoRaw(::grpc::ClientContext* context, const ::dc2::ChannelInfoReq& request, ::grpc::CompletionQueue* cq) {
  return ::grpc::ClientAsyncResponseReader< ::dc2::ChatChannel>::Create(channel_.get(), cq, rpcmethod_ChannelInfo_, context, request, false);
}

::grpc::Status ChatHub::Stub::SetChannelOptions(::grpc::ClientContext* context, const ::dc2::SetChannelOptionsReq& request, ::dc2::Empty* response) {
  return ::grpc::BlockingUnaryCall(channel_.get(), rpcmethod_SetChannelOptions_, context, request, response);
}

::grpc::ClientAsyncResponseReader< ::dc2::Empty>* ChatHub::Stub::AsyncSetChannelOptionsRaw(::grpc::ClientContext* context, const ::dc2::SetChannelOptionsReq& request, ::grpc::CompletionQueue* cq) {
  return ::grpc::ClientAsyncResponseReader< ::dc2::Empty>::Create(channel_.get(), cq, rpcmethod_SetChannelOptions_, context, request, true);
}

::grpc::ClientAsyncResponseReader< ::dc2::Empty>* ChatHub::Stub::PrepareAsyncSetChannelOptionsRaw(::grpc::ClientContext* context, const ::dc2::SetChannelOptionsReq& request, ::grpc::CompletionQueue* cq) {
  return ::grpc::ClientAsyncResponseReader< ::dc2::Empty>::Create(channel_.get(), cq, rpcmethod_SetChannelOptions_, context, request, false);
}

ChatHub::Service::Service() {
  AddMethod(new ::grpc::RpcServiceMethod(
      ChatHub_method_names[0],
      ::grpc::RpcMethod::SERVER_STREAMING,
      new ::grpc::ServerStreamingHandler< ChatHub::Service, ::dc2::ChannelsReq, ::dc2::ChatChannel>(
          std::mem_fn(&ChatHub::Service::Channels), this)));
  AddMethod(new ::grpc::RpcServiceMethod(
      ChatHub_method_names[1],
      ::grpc::RpcMethod::NORMAL_RPC,
      new ::grpc::RpcMethodHandler< ChatHub::Service, ::dc2::ChannelInfoReq, ::dc2::ChatChannel>(
          std::mem_fn(&ChatHub::Service::ChannelInfo), this)));
  AddMethod(new ::grpc::RpcServiceMethod(
      ChatHub_method_names[2],
      ::grpc::RpcMethod::NORMAL_RPC,
      new ::grpc::RpcMethodHandler< ChatHub::Service, ::dc2::SetChannelOptionsReq, ::dc2::Empty>(
          std::mem_fn(&ChatHub::Service::SetChannelOptions), this)));
}

ChatHub::Service::~Service() {
}

::grpc::Status ChatHub::Service::Channels(::grpc::ServerContext* context, const ::dc2::ChannelsReq* request, ::grpc::ServerWriter< ::dc2::ChatChannel>* writer) {
  (void) context;
  (void) request;
  (void) writer;
  return ::grpc::Status(::grpc::StatusCode::UNIMPLEMENTED, "");
}

::grpc::Status ChatHub::Service::ChannelInfo(::grpc::ServerContext* context, const ::dc2::ChannelInfoReq* request, ::dc2::ChatChannel* response) {
  (void) context;
  (void) request;
  (void) response;
  return ::grpc::Status(::grpc::StatusCode::UNIMPLEMENTED, "");
}

::grpc::Status ChatHub::Service::SetChannelOptions(::grpc::ServerContext* context, const ::dc2::SetChannelOptionsReq* request, ::dc2::Empty* response) {
  (void) context;
  (void) request;
  (void) response;
  return ::grpc::Status(::grpc::StatusCode::UNIMPLEMENTED, "");
}


static const char* Chat_method_names[] = {
  "/dc2.Chat/StreamMessages",
};

std::unique_ptr< Chat::Stub> Chat::NewStub(const std::shared_ptr< ::grpc::ChannelInterface>& channel, const ::grpc::StubOptions& options) {
  std::unique_ptr< Chat::Stub> stub(new Chat::Stub(channel));
  return stub;
}

Chat::Stub::Stub(const std::shared_ptr< ::grpc::ChannelInterface>& channel)
  : channel_(channel), rpcmethod_StreamMessages_(Chat_method_names[0], ::grpc::RpcMethod::BIDI_STREAMING, channel)
  {}

::grpc::ClientReaderWriter< ::dc2::ChatMessage, ::dc2::ChatMessage>* Chat::Stub::StreamMessagesRaw(::grpc::ClientContext* context) {
  return new ::grpc::ClientReaderWriter< ::dc2::ChatMessage, ::dc2::ChatMessage>(channel_.get(), rpcmethod_StreamMessages_, context);
}

::grpc::ClientAsyncReaderWriter< ::dc2::ChatMessage, ::dc2::ChatMessage>* Chat::Stub::AsyncStreamMessagesRaw(::grpc::ClientContext* context, ::grpc::CompletionQueue* cq, void* tag) {
  return ::grpc::ClientAsyncReaderWriter< ::dc2::ChatMessage, ::dc2::ChatMessage>::Create(channel_.get(), cq, rpcmethod_StreamMessages_, context, true, tag);
}

::grpc::ClientAsyncReaderWriter< ::dc2::ChatMessage, ::dc2::ChatMessage>* Chat::Stub::PrepareAsyncStreamMessagesRaw(::grpc::ClientContext* context, ::grpc::CompletionQueue* cq) {
  return ::grpc::ClientAsyncReaderWriter< ::dc2::ChatMessage, ::dc2::ChatMessage>::Create(channel_.get(), cq, rpcmethod_StreamMessages_, context, false, nullptr);
}

Chat::Service::Service() {
  AddMethod(new ::grpc::RpcServiceMethod(
      Chat_method_names[0],
      ::grpc::RpcMethod::BIDI_STREAMING,
      new ::grpc::BidiStreamingHandler< Chat::Service, ::dc2::ChatMessage, ::dc2::ChatMessage>(
          std::mem_fn(&Chat::Service::StreamMessages), this)));
}

Chat::Service::~Service() {
}

::grpc::Status Chat::Service::StreamMessages(::grpc::ServerContext* context, ::grpc::ServerReaderWriter< ::dc2::ChatMessage, ::dc2::ChatMessage>* stream) {
  (void) context;
  (void) stream;
  return ::grpc::Status(::grpc::StatusCode::UNIMPLEMENTED, "");
}


static const char* Identity_method_names[] = {
  "/dc2.Identity/Info",
};

std::unique_ptr< Identity::Stub> Identity::NewStub(const std::shared_ptr< ::grpc::ChannelInterface>& channel, const ::grpc::StubOptions& options) {
  std::unique_ptr< Identity::Stub> stub(new Identity::Stub(channel));
  return stub;
}

Identity::Stub::Stub(const std::shared_ptr< ::grpc::ChannelInterface>& channel)
  : channel_(channel), rpcmethod_Info_(Identity_method_names[0], ::grpc::RpcMethod::NORMAL_RPC, channel)
  {}

::grpc::Status Identity::Stub::Info(::grpc::ClientContext* context, const ::dc2::Empty& request, ::dc2::User* response) {
  return ::grpc::BlockingUnaryCall(channel_.get(), rpcmethod_Info_, context, request, response);
}

::grpc::ClientAsyncResponseReader< ::dc2::User>* Identity::Stub::AsyncInfoRaw(::grpc::ClientContext* context, const ::dc2::Empty& request, ::grpc::CompletionQueue* cq) {
  return ::grpc::ClientAsyncResponseReader< ::dc2::User>::Create(channel_.get(), cq, rpcmethod_Info_, context, request, true);
}

::grpc::ClientAsyncResponseReader< ::dc2::User>* Identity::Stub::PrepareAsyncInfoRaw(::grpc::ClientContext* context, const ::dc2::Empty& request, ::grpc::CompletionQueue* cq) {
  return ::grpc::ClientAsyncResponseReader< ::dc2::User>::Create(channel_.get(), cq, rpcmethod_Info_, context, request, false);
}

Identity::Service::Service() {
  AddMethod(new ::grpc::RpcServiceMethod(
      Identity_method_names[0],
      ::grpc::RpcMethod::NORMAL_RPC,
      new ::grpc::RpcMethodHandler< Identity::Service, ::dc2::Empty, ::dc2::User>(
          std::mem_fn(&Identity::Service::Info), this)));
}

Identity::Service::~Service() {
}

::grpc::Status Identity::Service::Info(::grpc::ServerContext* context, const ::dc2::Empty* request, ::dc2::User* response) {
  (void) context;
  (void) request;
  (void) response;
  return ::grpc::Status(::grpc::StatusCode::UNIMPLEMENTED, "");
}


static const char* UsersHub_method_names[] = {
  "/dc2.UsersHub/UserInfo",
  "/dc2.UsersHub/UpdateInfo",
  "/dc2.UsersHub/Users",
};

std::unique_ptr< UsersHub::Stub> UsersHub::NewStub(const std::shared_ptr< ::grpc::ChannelInterface>& channel, const ::grpc::StubOptions& options) {
  std::unique_ptr< UsersHub::Stub> stub(new UsersHub::Stub(channel));
  return stub;
}

UsersHub::Stub::Stub(const std::shared_ptr< ::grpc::ChannelInterface>& channel)
  : channel_(channel), rpcmethod_UserInfo_(UsersHub_method_names[0], ::grpc::RpcMethod::NORMAL_RPC, channel)
  , rpcmethod_UpdateInfo_(UsersHub_method_names[1], ::grpc::RpcMethod::NORMAL_RPC, channel)
  , rpcmethod_Users_(UsersHub_method_names[2], ::grpc::RpcMethod::SERVER_STREAMING, channel)
  {}

::grpc::Status UsersHub::Stub::UserInfo(::grpc::ClientContext* context, const ::dc2::SIDMsg& request, ::dc2::User* response) {
  return ::grpc::BlockingUnaryCall(channel_.get(), rpcmethod_UserInfo_, context, request, response);
}

::grpc::ClientAsyncResponseReader< ::dc2::User>* UsersHub::Stub::AsyncUserInfoRaw(::grpc::ClientContext* context, const ::dc2::SIDMsg& request, ::grpc::CompletionQueue* cq) {
  return ::grpc::ClientAsyncResponseReader< ::dc2::User>::Create(channel_.get(), cq, rpcmethod_UserInfo_, context, request, true);
}

::grpc::ClientAsyncResponseReader< ::dc2::User>* UsersHub::Stub::PrepareAsyncUserInfoRaw(::grpc::ClientContext* context, const ::dc2::SIDMsg& request, ::grpc::CompletionQueue* cq) {
  return ::grpc::ClientAsyncResponseReader< ::dc2::User>::Create(channel_.get(), cq, rpcmethod_UserInfo_, context, request, false);
}

::grpc::Status UsersHub::Stub::UpdateInfo(::grpc::ClientContext* context, const ::dc2::User& request, ::dc2::SIDMsg* response) {
  return ::grpc::BlockingUnaryCall(channel_.get(), rpcmethod_UpdateInfo_, context, request, response);
}

::grpc::ClientAsyncResponseReader< ::dc2::SIDMsg>* UsersHub::Stub::AsyncUpdateInfoRaw(::grpc::ClientContext* context, const ::dc2::User& request, ::grpc::CompletionQueue* cq) {
  return ::grpc::ClientAsyncResponseReader< ::dc2::SIDMsg>::Create(channel_.get(), cq, rpcmethod_UpdateInfo_, context, request, true);
}

::grpc::ClientAsyncResponseReader< ::dc2::SIDMsg>* UsersHub::Stub::PrepareAsyncUpdateInfoRaw(::grpc::ClientContext* context, const ::dc2::User& request, ::grpc::CompletionQueue* cq) {
  return ::grpc::ClientAsyncResponseReader< ::dc2::SIDMsg>::Create(channel_.get(), cq, rpcmethod_UpdateInfo_, context, request, false);
}

::grpc::ClientReader< ::dc2::User>* UsersHub::Stub::UsersRaw(::grpc::ClientContext* context, const ::dc2::UsersReq& request) {
  return new ::grpc::ClientReader< ::dc2::User>(channel_.get(), rpcmethod_Users_, context, request);
}

::grpc::ClientAsyncReader< ::dc2::User>* UsersHub::Stub::AsyncUsersRaw(::grpc::ClientContext* context, const ::dc2::UsersReq& request, ::grpc::CompletionQueue* cq, void* tag) {
  return ::grpc::ClientAsyncReader< ::dc2::User>::Create(channel_.get(), cq, rpcmethod_Users_, context, request, true, tag);
}

::grpc::ClientAsyncReader< ::dc2::User>* UsersHub::Stub::PrepareAsyncUsersRaw(::grpc::ClientContext* context, const ::dc2::UsersReq& request, ::grpc::CompletionQueue* cq) {
  return ::grpc::ClientAsyncReader< ::dc2::User>::Create(channel_.get(), cq, rpcmethod_Users_, context, request, false, nullptr);
}

UsersHub::Service::Service() {
  AddMethod(new ::grpc::RpcServiceMethod(
      UsersHub_method_names[0],
      ::grpc::RpcMethod::NORMAL_RPC,
      new ::grpc::RpcMethodHandler< UsersHub::Service, ::dc2::SIDMsg, ::dc2::User>(
          std::mem_fn(&UsersHub::Service::UserInfo), this)));
  AddMethod(new ::grpc::RpcServiceMethod(
      UsersHub_method_names[1],
      ::grpc::RpcMethod::NORMAL_RPC,
      new ::grpc::RpcMethodHandler< UsersHub::Service, ::dc2::User, ::dc2::SIDMsg>(
          std::mem_fn(&UsersHub::Service::UpdateInfo), this)));
  AddMethod(new ::grpc::RpcServiceMethod(
      UsersHub_method_names[2],
      ::grpc::RpcMethod::SERVER_STREAMING,
      new ::grpc::ServerStreamingHandler< UsersHub::Service, ::dc2::UsersReq, ::dc2::User>(
          std::mem_fn(&UsersHub::Service::Users), this)));
}

UsersHub::Service::~Service() {
}

::grpc::Status UsersHub::Service::UserInfo(::grpc::ServerContext* context, const ::dc2::SIDMsg* request, ::dc2::User* response) {
  (void) context;
  (void) request;
  (void) response;
  return ::grpc::Status(::grpc::StatusCode::UNIMPLEMENTED, "");
}

::grpc::Status UsersHub::Service::UpdateInfo(::grpc::ServerContext* context, const ::dc2::User* request, ::dc2::SIDMsg* response) {
  (void) context;
  (void) request;
  (void) response;
  return ::grpc::Status(::grpc::StatusCode::UNIMPLEMENTED, "");
}

::grpc::Status UsersHub::Service::Users(::grpc::ServerContext* context, const ::dc2::UsersReq* request, ::grpc::ServerWriter< ::dc2::User>* writer) {
  (void) context;
  (void) request;
  (void) writer;
  return ::grpc::Status(::grpc::StatusCode::UNIMPLEMENTED, "");
}


}  // namespace dc2
