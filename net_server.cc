#include <iostream>
#include <memory>
#include <string>
#include <unordered_map>
#include <queue>
#include <vector>
#include <deque>
#include <mutex>
#include <thread>


#include <grpcpp/grpcpp.h>

#ifdef BAZEL_BUILD
#include "examples/protos/socialnetwork.grpc.pb.h"
#else

#include "socialnetwork.pb.h"
#include "socialnetwork.grpc.pb.h"

#endif

using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::Status;
using socialnetwork::SocialNetwork;
using socialnetwork::AddUserRequest;
using socialnetwork::AddFriendRequest;
using socialnetwork::ChainRequest;
using socialnetwork::Response;

class Storage {
public:

    bool add_user(const AddUserRequest *user) {
        {
            std::lock_guard<std::mutex> lock2(graph_mtx);
            if (graph.find(user->tag()) != graph.end()) {
                return false;
            }
            graph[user->tag()];
        }
        std::lock_guard<std::mutex> lock1(storage_mtx);
        UserData user_data(user->name());
        storage.insert(std::pair<std::string, UserData>(user->tag(), user_data));
        return true;
    }

    bool add_friend(const std::string &tag1, const std::string &tag2) {
        std::lock_guard<std::mutex> lock(graph_mtx);
        if (graph.find(tag1) == graph.end() || graph.find(tag2) == graph.end()) {
            return false;
        }
        // check whether they are already friends
        graph[tag1].push_back(tag2);
        graph[tag2].push_back(tag1);
        return true;
    }

    std::string tracing_bfs(const std::string &tag1, const std::string &tag2) {
        std::lock_guard<std::mutex> lock(graph_mtx);
        if (graph.find(tag1) == graph.end() || graph.find(tag2) == graph.end()) {
            return "";
        }
        std::string output;
        std::deque<std::deque<std::string>> queue;
        queue.push_back({tag1});

        while (!queue.empty()) {
            auto path = queue.front();
            queue.pop_front();
            std::string last_node = path[path.size()-1];
            if (last_node == tag2) {
                for (const auto &str: path) {
                    output += str + " ---> ";
                }
                return output;
            }
            for (const auto &adj: graph.find(last_node)->second) {
                std::deque<std::string> new_path = {path};
                new_path.push_back(adj);
                queue.push_back(new_path);
            }
        }
        return "";
    }

private:
    struct UserData {
        std::string name;
        UserData(const std::string &username) : name(username) {}
    };
    std::unordered_map<std::string, std::vector<std::string>> graph;
    std::unordered_map<std::string, UserData> storage;
    std::mutex graph_mtx, storage_mtx;
};

// Logic and data behind the server's behavior.
class SNService : public SocialNetwork::Service {
    Status AddUser(ServerContext *context, const AddUserRequest *request,
                   Response *reply) override {
        // Overwrite the call's compression algorithm to DEFLATE.
        context->set_compression_algorithm(GRPC_COMPRESS_DEFLATE);
        auto result = storage.add_user(request);
        if (!result) {
            reply->set_return_message("This user is already registered");
            return Status::OK;
        }
        std::string prefix("User has been successfully added: ");
        reply->set_return_message(prefix + request->name() + " " + request->tag());
        return Status::OK;
    }

    Status AddFriend(ServerContext *context, const AddFriendRequest *request,
                     Response *reply) override {
        // Overwrite the call's compression algorithm to DEFLATE.
        context->set_compression_algorithm(GRPC_COMPRESS_DEFLATE);
        auto result = storage.add_friend(request->first_tag(), request->second_tag());
        if (!result) {
            reply->set_return_message("Couldn't complete the request. One of the users is not present");
            return Status::OK;
        }
        std::string prefix("The following users are now friends: ");
        reply->set_return_message(prefix + request->first_tag() + ", " + request->second_tag());
        return Status::OK;
    }

    Status GetFriendChain(ServerContext *context, const ChainRequest *request,
                          Response *reply) override {

        context->set_compression_algorithm(GRPC_COMPRESS_DEFLATE);
        std::string result = storage.tracing_bfs(request->first_tag(), request->second_tag());
        std::string prefix("User chain: ");
        reply->set_return_message(prefix + result);
        return Status::OK;
    }

private:
    Storage storage;
};

void RunServer() {
    std::string server_address("0.0.0.0:50051");
    SNService service;

    ServerBuilder builder;
    // Set the default compression algorithm for the server.
    builder.SetDefaultCompressionAlgorithm(GRPC_COMPRESS_GZIP);
    // Listen on the given address without any authentication mechanism.
    builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
    // Register "service" as the instance through which we'll communicate with
    // clients. In this case it corresponds to an *synchronous* service.
    builder.RegisterService(&service);
    // Finally assemble the server.
    std::unique_ptr<Server> server(builder.BuildAndStart());
    std::cout << "Server listening on " << server_address << std::endl;

    // Wait for the server to shutdown. Note that some other thread must be
    // responsible for shutting down the server for this call to ever return.
    server->Wait();
}

int main(int argc, char **argv) {
    RunServer();

    return 0;
}
