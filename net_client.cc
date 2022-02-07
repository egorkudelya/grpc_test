//
// Created by Egor Kudelia on 03.02.2022.
//

/*
 *
 * Copyright 2018 gRPC authors.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */

#include <iostream>
#include <memory>
#include <string>

#include <grpcpp/grpcpp.h>

#ifdef BAZEL_BUILD
#include "examples/protos/socialnetwork.grpc.pb.h"
#else

#include "socialnetwork.grpc.pb.h"

#endif

using grpc::Channel;
using grpc::ChannelArguments;
using grpc::ClientContext;
using grpc::Status;
using socialnetwork::SocialNetwork;
using socialnetwork::AddUserRequest;
using socialnetwork::AddFriendRequest;
using socialnetwork::ChainRequest;
using socialnetwork::Response;

class Client {
public:
    Client(std::shared_ptr<Channel> channel)
            : stub_(SocialNetwork::NewStub(channel)) {}

    // Assembles the client's payload, sends it and presents the response back
    // from the server.
    std::string AddUser(const std::string &user_name, const std::string &tag) {
        // Data we are sending to the server.
        AddUserRequest request;
        request.set_name(user_name);
        request.set_tag(tag);

        // Container for the data we expect from the server.
        Response reply;

        // Context for the client. It could be used to convey extra information to
        // the server and/or tweak certain RPC behaviors.
        ClientContext context;

        // Overwrite the call's compression algorithm to DEFLATE.
        context.set_compression_algorithm(GRPC_COMPRESS_DEFLATE);

        // The actual RPC.
        Status status = stub_->AddUser(&context, request, &reply);

        // Act upon its status.
        if (status.ok()) {
            return reply.return_message();
        } else {
            std::cout << status.error_code() << ": " << status.error_message()
                      << std::endl;
            return "RPC failed";
        }
    }

    std::string AddFriend(const std::string &tag1, const std::string &tag2) {
        // Data we are sending to the server.
        AddFriendRequest request;
        request.set_first_tag(tag1);
        request.set_second_tag(tag2);

        // Container for the data we expect from the server.
        Response reply;

        // Context for the client. It could be used to convey extra information to
        // the server and/or tweak certain RPC behaviors.
        ClientContext context;

        // Overwrite the call's compression algorithm to DEFLATE.
        context.set_compression_algorithm(GRPC_COMPRESS_DEFLATE);

        // The actual RPC.
        Status status = stub_->AddFriend(&context, request, &reply);

        // Act upon its status.
        if (status.ok()) {
            return reply.return_message();
        } else {
            std::cout << status.error_code() << ": " << status.error_message()
                      << std::endl;
            return "RPC failed";
        }
    }

    std::string GetFriendChain(const std::string &tag1, const std::string &tag2) {
        ChainRequest request;
        request.set_first_tag(tag1);
        request.set_second_tag(tag2);

        Response reply;
        ClientContext context;

        context.set_compression_algorithm(GRPC_COMPRESS_DEFLATE);
        Status status = stub_->GetFriendChain(&context, request, &reply);

        if (status.ok()) {
            return reply.return_message();
        } else {
            std::cout << status.error_code() << ": " << status.error_message()
                      << std::endl;
            return "RPC failed";
        }
    }

private:
    std::unique_ptr<SocialNetwork::Stub> stub_;
};

int main(int argc, char **argv) {
    ChannelArguments args;

    args.SetCompressionAlgorithm(GRPC_COMPRESS_GZIP);
    Client client(grpc::CreateCustomChannel("localhost:50051", grpc::InsecureChannelCredentials(), args));

    std::vector<std::string> names = {"Katerine", "Julia", "Conor", "Joe", "Dominica"};
    std::vector<std::string> tags = {"@washington", "@lia", "@rand123", "@idk32", "@monica90"};

    std::string reply;
    for(int i = 0; i < 5; i++) {
        reply = client.AddUser(names[i], tags[i]);
        std::cout << "Reply: " << reply << std::endl;
    }

    reply = client.AddUser(names[3], tags[3]);
    std::cout << "Reply: " << reply << std::endl;

    reply = client.AddFriend("@monica90", "@lia");
    std::cout << "Reply: " << reply << std::endl;

    reply = client.AddFriend("@lia", "@washington");
    std::cout << "Reply: " << reply << std::endl;

    reply = client.AddFriend("@rand123", "@washington");
    std::cout << "Reply: " << reply << std::endl;

    reply = client.GetFriendChain("@rand123", "@monica90");
    std::cout << reply << std::endl;

    return 0;
}
