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

#pragma once

#include <atomic>
#include <map>

#include "ChunkQueue.hpp"
#include "DataChannel.hpp"

#ifdef __MINGW32__
#define EXPORT __attribute__((dllexport))
#else
#define EXPORT
#endif //__MINGW32__

namespace rtcdcpp {

  class NiceWrapper;
  class DTLSWrapper;
  class SCTPWrapper;

  struct RTCIceServer {
      RTCIceServer()
          : hostname_()
          , port_(0)
      {
      }
      RTCIceServer(const std::string &hostname, int port)
          : hostname_(hostname)
          , port_(port)
      {
      }

    std::string hostname_;
    int port_;
  };

  std::ostream &operator<<(std::ostream &os, const RTCIceServer &ice_server);

  enum class IceServerType {
      STUN
      , TURN
  };

  struct RTCConfiguration {
      RTCConfiguration()
          : type(IceServerType::STUN)
          , ice_servers()
          , ice_ufrag("")
          , ice_pwd("")
      {
      }
      RTCConfiguration(IceServerType serverType
                       , const std::vector<RTCIceServer> &servers
                       , std::string ufrag = ""
                       , std::string pwd = "")
          : type(serverType)
          , ice_servers(servers)
          , ice_ufrag(std::move(ufrag))
          , ice_pwd(std::move(pwd))
      {
      }

    IceServerType type;
    std::vector<RTCIceServer> ice_servers;
    std::string ice_ufrag;
    std::string ice_pwd;
  };

  using IceConfig = std::vector<RTCConfiguration>;

  class EXPORT PeerConnection {
    friend class DTLSWrapper;
    friend class DataChannel;
    public:
    struct IceCandidate {
      IceCandidate(const std::string &candidate, const std::string &sdpMid, int sdpMLineIndex)
        : candidate(candidate), sdpMid(sdpMid), sdpMLineIndex(sdpMLineIndex) {}
      std::string candidate;
      std::string sdpMid;
      int sdpMLineIndex;
    };

    using IceCandidateCallbackPtr = std::function<void(IceCandidate)>;
    using DataChannelCallbackPtr = std::function<void(std::shared_ptr<DataChannel> channel)>;

    PeerConnection(const IceConfig &config, IceCandidateCallbackPtr icCB, DataChannelCallbackPtr dcCB);

    virtual ~PeerConnection();

    const IceConfig& Config() const noexcept { return config_; }

    /**
     *
     * Parse Offer SDP
     */
    void ParseOffer(std::string offer_sdp);

    /**
     * Generate Offer SDP
     */
    std::string GenerateOffer();

    /**
     * Generate Answer SDP
     */
    std::string GenerateAnswer();

    /**
     * Handle remote ICE Candidate.
     * Supports trickle ice candidates.
     */
    bool SetRemoteIceCandidate(const std::string &candidate_sdp);

    /**
     * Handle remote ICE Candidates.
     * TODO: Handle trickle ice candidates.
     */
    bool SetRemoteIceCandidates(const std::vector<std::string> &candidate_sdps);

    /**
     * Create a new data channel with the given label.
     * Only callable once RTCConnectedCallback has been called.
     * TODO: Handle creating data channels before generating SDP, so that the
     *       data channel is created as part of the connection process.
     */
    std::shared_ptr<DataChannel> CreateDataChannel(std::string label, std::string protocol="", uint8_t chan_type=DATA_CHANNEL_RELIABLE, uint32_t reliability=0);

    /**
     * Notify when remote party creates a DataChannel.
     * XXX: This is *not* a callback saying that a call to CreateDataChannel
     *      has succeeded. This is a call saying the remote party wants to
     *      create a new data channel.
     */
    //  void SetDataChannelCreatedCallback(DataChannelCallbackPtr cb);

    // TODO: Error callbacks

    void SendStrMsg(std::string msg, uint16_t sid);
    void SendBinaryMsg(const uint8_t *data, int len, uint16_t sid);
    void StopSendData();

    /* Internal Callback Handlers */
    void OnLocalIceCandidate(std::string &ice_candidate);
    void OnIceReady();
    void OnDTLSHandshakeDone();
    void OnSCTPMsgReceived(ChunkPtr chunk, uint16_t sid, uint32_t ppid);

    private:
    IceConfig config_;
    const IceCandidateCallbackPtr ice_candidate_cb;
    const DataChannelCallbackPtr new_channel_cb;
    std::string mid;

    enum Role { Client, Server } role = Client;

    std::atomic<bool> iceReady{false};
    std::unique_ptr<NiceWrapper> nice;
    std::unique_ptr<DTLSWrapper> dtls;
    std::unique_ptr<SCTPWrapper> sctp;

    std::map<uint16_t, std::shared_ptr<DataChannel>> data_channels;
    std::shared_ptr<DataChannel> GetChannel(uint16_t sid);

    /**
     * Constructor helper
     * Initialize the RTC connection.
     * Allocates all internal structures and configs, and starts ICE gathering.
     */
    bool Initialize();

    // DataChannel message parsing
    void HandleNewDataChannel(ChunkPtr chunk, uint16_t sid);
    void HandleDataChannelAck(uint16_t sid);
    void HandleDataChannelClose(uint16_t sid);
    void HandleStringMessage(ChunkPtr chunk, uint16_t sid);
    void HandleBinaryMessage(ChunkPtr chunk, uint16_t sid);

    void ResetSCTPStream(uint16_t stream_id);
  };
}
