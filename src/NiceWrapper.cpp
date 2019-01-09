/**
 * Copyright (c) 2017, Andrew Gault, Nick Chadwick and Guillaume Egles.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *    * Redistributions of source code must retain the above copyright
 *      notice, this list of conditions and the following disclaimer.
 *    * Redistributions in binary form must reproduce the above copyright
 *      notice, this list of conditions and the following disclaimer in the
 *      documentation and/or other materials provided with the distribution.
 *    * Neither the name of the <organization> nor the
 *      names of its contributors may be used to endorse or promote products
 *      derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDERS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/**
 * Basic implementation of libnice stuff.
 */
#include <sstream>
#ifndef __WIN32
#include <netdb.h>
#else
#include <winsock2.h>
#endif // __WIN32

#include "NiceWrapper.hpp"

 void ReplaceAll(std::string &s, const std::string &search, const std::string &replace) {
   size_t pos = 0;
   while ((pos = s.find(search, pos)) != std::string::npos) {
     s.replace(pos, search.length(), replace);
     pos += replace.length();
   }
 }

 const char* GetIpByHostname(const char *hostname)
 {
     struct hostent *stun_host = gethostbyname(hostname);
     if (stun_host == nullptr) {
         //std::cerr << "Failed to lookup host for server: " << hostname << '\n';
         return nullptr;
     } else {
         in_addr *address = (in_addr *)stun_host->h_addr;
         return inet_ntoa(*address);
     }
 }

 namespace rtcdcpp {

 using namespace std;

 NiceWrapper::NiceWrapper(PeerConnection *peer_connection)
     : peer_connection(peer_connection), stream_id(0), should_stop(false), send_queue(), agent(NULL, nullptr), loop(NULL, nullptr), context(NULL, nullptr), packets_sent(0) {
   data_received_callback = [](ChunkPtr x) { ; };
   nice_debug_disable(true);
 }

 NiceWrapper::~NiceWrapper() { Stop(); }

 void new_local_candidate(NiceAgent *agent, NiceCandidate *candidate, gpointer user_data) {
   NiceWrapper *nice = (NiceWrapper *)user_data;
   gchar *cand = nice_agent_generate_local_candidate_sdp(agent, candidate);
   std::string cand_str(cand);
   nice->OnCandidate(cand_str);
   g_free(cand);
 }

 void NiceWrapper::OnCandidate(std::string candidate) {
   this->peer_connection->OnLocalIceCandidate(candidate);
 }

 void candidate_gathering_done(NiceAgent *agent, guint stream_id, gpointer user_data) {
   NiceWrapper *nice = (NiceWrapper *)user_data;
   nice->OnGatheringDone();
 }

 // TODO: Callback for this
 void NiceWrapper::OnGatheringDone() {
   std::string empty_candidate("");
   this->peer_connection->OnLocalIceCandidate(empty_candidate);
 }

 // TODO: Callbacks on failure
 void component_state_changed(NiceAgent *agent, guint stream_id, guint component_id, guint state, gpointer user_data) {
   NiceWrapper *nice = (NiceWrapper *)user_data;
   nice->OnStateChange(stream_id, component_id, state);
 }

 void NiceWrapper::OnStateChange(uint32_t stream_id, uint32_t component_id, uint32_t state) {
   switch (state) {
     case (NICE_COMPONENT_STATE_DISCONNECTED):
       break;
     case (NICE_COMPONENT_STATE_GATHERING):
       break;
     case (NICE_COMPONENT_STATE_CONNECTING):
       break;
     case (NICE_COMPONENT_STATE_CONNECTED):
       break;
     case (NICE_COMPONENT_STATE_READY):
       this->OnIceReady();
       break;
     case (NICE_COMPONENT_STATE_FAILED):
       break;
     default:
       break;
   }
 }

 // TODO: Turn this into a callback
 void NiceWrapper::OnIceReady() { this->peer_connection->OnIceReady(); }

 void new_selected_pair(NiceAgent *agent, guint stream_id, guint component_id, NiceCandidate *lcandidate, NiceCandidate *rcandidate,
                        gpointer user_data) {
   NiceWrapper *nice = (NiceWrapper *)user_data;
   nice->OnSelectedPair();
 }

 void NiceWrapper::OnSelectedPair() {}

 void data_received(NiceAgent *agent, guint stream_id, guint component_id, guint len, gchar *buf, gpointer user_data) {
   NiceWrapper *nice = (NiceWrapper *)user_data;
   nice->OnDataReceived((const uint8_t *)buf, len);
 }

 void NiceWrapper::OnDataReceived(const uint8_t *buf, int len) {
   this->data_received_callback(std::make_shared<Chunk>(buf, len));
 }

 void nice_log_handler(const gchar *log_domain, GLogLevelFlags log_level, const gchar *message, gpointer user_data) {
   NiceWrapper *nice = (NiceWrapper *)user_data;
   nice->LogMessage(message);
 }

 void NiceWrapper::LogMessage(const gchar *message) {}

 bool NiceWrapper::Initialize() {
   int log_flags = G_LOG_LEVEL_MASK | G_LOG_FLAG_FATAL | G_LOG_FLAG_RECURSION;
   g_log_set_handler(NULL, (GLogLevelFlags)log_flags, nice_log_handler, this);

   this->context = std::unique_ptr<GMainContext, void (*)(GMainContext *)>(g_main_context_new(), g_main_context_unref);
   if (!this->context) {
    std::cerr << "Failed to initialize GMainContext\n";
    return false;
   }

   this->loop = std::unique_ptr<GMainLoop, void (*)(GMainLoop *)>(g_main_loop_new(context.get(), FALSE), g_main_loop_unref);
   if (!this->loop) {
    std::cerr << "Failed to initialize GMainLoop\n";
    return false;
   }

   this->agent = std::unique_ptr<NiceAgent, decltype(&g_object_unref)>(nice_agent_new(g_main_loop_get_context(loop.get()), NICE_COMPATIBILITY_RFC5245),
                                                                       g_object_unref);
   if (!this->agent) {
     std::cerr << "Failed to initialize nice agent\n";
     return false;
   }

   this->g_main_loop_thread = std::thread(g_main_loop_run, this->loop.get());

   g_object_set(G_OBJECT(agent.get()), "upnp", FALSE, NULL);
   g_object_set(G_OBJECT(agent.get()), "controlling-mode", 0, NULL);

   // TODO: Learn more about nice streams
   this->stream_id = nice_agent_add_stream(agent.get(), 1);
   if (this->stream_id == 0) {
     return false;
   }

   nice_agent_set_stream_name(agent.get(), this->stream_id, "application");

   for(const auto &conf : peer_connection->Config()) {
       if(IceServerType::STUN == conf.type) {
           AddStunServers(conf);
       }
       else if(IceServerType::TURN == conf.type) {
           AddTurnServers(conf);
       }
       else {
           continue;
       }
   }

   g_signal_connect(G_OBJECT(agent.get()), "candidate-gathering-done", G_CALLBACK(candidate_gathering_done), this);
   g_signal_connect(G_OBJECT(agent.get()), "component-state-changed", G_CALLBACK(component_state_changed), this);
   g_signal_connect(G_OBJECT(agent.get()), "new-candidate-full", G_CALLBACK(new_local_candidate), this);
   g_signal_connect(G_OBJECT(agent.get()), "new-selected-pair", G_CALLBACK(new_selected_pair), this);

   if(!nice_agent_attach_recv(agent.get(), this->stream_id, 1, g_main_loop_get_context(loop.get()), data_received, this)) {
       return false;
   }

   return true;
 }

 void NiceWrapper::AddStunServers(const RTCConfiguration &config)
 {
     if (config.ice_servers.size() > 1) {
       throw std::invalid_argument("Only up to one ICE server is currently supported\n");
     }

     for (const auto &ice_server : config.ice_servers) {
         const char *ipAddr = GetIpByHostname(ice_server.hostname_.c_str());
         if(nullptr != ipAddr) {
             g_object_set(G_OBJECT(agent.get()), "stun-server", ipAddr, NULL);
         }
         if (ice_server.port_ > 0) {
             g_object_set(G_OBJECT(agent.get()), "stun-server-port", ice_server.port_, NULL);
         } else {
             //std::cerr << "stun port empty\n";
         }
     }

     if (!config.ice_ufrag.empty() && !config.ice_pwd.empty()) {
         nice_agent_set_local_credentials(agent.get(), this->stream_id, config.ice_ufrag.c_str(), config.ice_pwd.c_str());
     }
 }

 void NiceWrapper::AddTurnServers(const RTCConfiguration &config)
 {
    for(const auto &turnServ : config.ice_servers) {
        const char *ipAddr = GetIpByHostname(turnServ.hostname_.c_str());
        if(nullptr != ipAddr
           && 0 < turnServ.port_
           && !config.ice_ufrag.empty()
           && !config.ice_pwd.empty() )
        {
            nice_agent_set_relay_info(this->agent.get()
                                      , this->stream_id
                                      , 1
                                      , ipAddr
                                      , turnServ.port_
                                      , config.ice_ufrag.c_str()
                                      , config.ice_pwd.c_str()
                                      , NICE_RELAY_TYPE_TURN_UDP
                                      );
        }
    }
 }

 void NiceWrapper::StartSendLoop() { this->send_thread = std::thread(&NiceWrapper::SendLoop, this); }

 void NiceWrapper::Stop() {
   this->should_stop = true;

   send_queue.Stop();
   if (this->send_thread.joinable()) {
     this->send_thread.join();
   }

   g_main_loop_quit(this->loop.get());

   if (this->g_main_loop_thread.joinable()) {
     this->g_main_loop_thread.join();
   }
 }

 void NiceWrapper::ParseRemoteSDP(std::string remote_sdp) {
   string crfree_remote_sdp = remote_sdp;

   // TODO: Improve this. This is needed because otherwise libnice will wrongly take the '\r' as part of ice-ufrag/password.
   ReplaceAll(crfree_remote_sdp, "\r\n", "\n");

   int rc = nice_agent_parse_remote_sdp(this->agent.get(), crfree_remote_sdp.c_str());

   if (rc < 0) {
     throw std::runtime_error("ParseRemoteSDP: " + std::string(strerror(rc)));
   }

   if (!nice_agent_gather_candidates(agent.get(), this->stream_id)) {
     throw std::runtime_error("ParseRemoteSDP: Error gathering candidates!");
   }
 }

 void NiceWrapper::SendData(ChunkPtr chunk) {
   if (this->stream_id == 0) {
    //std::cerr << "ICE: ERROR sending data to unitialized nice context\n";
     return;
   }

   this->send_queue.push(chunk);
 }

 // Pull items off the send queue and call nice_agent_send
 void NiceWrapper::SendLoop() {
   while (!this->should_stop) {
     ChunkPtr chunk = send_queue.wait_and_pop();
     if (!chunk) {
       return;
     }
     size_t cur_len = chunk->Length();
     int result = 0;
     result = nice_agent_send(this->agent.get(), this->stream_id, 1, (guint)cur_len, (const char *)chunk->Data());
     if (result != cur_len) {
      //std::cerr << "ICE: Failed to send data\n";
     } else {
       // std::cerr << "ICE: Data sent " << cur_len << std::endl;
     }
   }
 }

 std::string NiceWrapper::GenerateLocalSDP() {
   std::stringstream nice_sdp;
   std::stringstream result;
   std::string line;

   gchar *raw_sdp = nice_agent_generate_local_sdp(agent.get());
   nice_sdp << raw_sdp;

   while (std::getline(nice_sdp, line)) {
     if (g_str_has_prefix(line.c_str(), "a=ice-ufrag:") || g_str_has_prefix(line.c_str(), "a=ice-pwd:")
           || g_str_has_prefix(line.c_str(), "a=candidate:")) {
       result << line << "\r\n";
     }
   }
   g_free(raw_sdp);
   return result.str();
 }

 bool NiceWrapper::SetRemoteIceCandidate(string candidate_sdp) {
   GSList *list = NULL;
   NiceCandidate *rcand = nice_agent_parse_remote_candidate_sdp(this->agent.get(), this->stream_id, candidate_sdp.c_str());

   if (rcand == NULL) {
    //std::cerr << "failed to parse remote candidate\n";
     return false;
   }
   list = g_slist_append(list, rcand);

   bool success = (nice_agent_set_remote_candidates(this->agent.get(), this->stream_id, 1, list) > 0);

   g_slist_free_full(list, (GDestroyNotify)&nice_candidate_free);

   return success;
 }

 bool NiceWrapper::SetRemoteIceCandidates(vector<string> candidate_sdps) {
   GSList *list = NULL;
   for (auto candidate_sdp : candidate_sdps) {
     NiceCandidate *rcand = nice_agent_parse_remote_candidate_sdp(this->agent.get(), this->stream_id, candidate_sdp.c_str());

     if (rcand == NULL) {
      //std::cerr << "failed to parse remote candidate\n";
       return false;
     }
     list = g_slist_append(list, rcand);
   }

   bool success = (nice_agent_set_remote_candidates(this->agent.get(), this->stream_id, 1, list) > 0);

   g_slist_free_full(list, (GDestroyNotify)&nice_candidate_free);

   return success;
 }

 void NiceWrapper::SetDataReceivedCallback(std::function<void(ChunkPtr)> data_received_callback) {
   this->data_received_callback = data_received_callback;
 }
}
