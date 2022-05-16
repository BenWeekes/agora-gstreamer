/*************************************************************
 *
 * This is a part of the Agora RTC Service SDK.
 * Copyright (C) 2020 Agora IO
 * All rights reserved.
 *
 *************************************************************/

#ifndef __AGORA_RTC_API_H__
#define __AGORA_RTC_API_H__

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#if defined(_MSC_VER)
#if defined(__BUILDING_AGORA_SDK__)
#define __agora_api__ __declspec(dllexport)
#else
#define __agora_api__ __declspec(dllimport)
#endif
#else
#define __agora_api__
#endif

#define AGORA_RTM_UID_MAX_LEN 64
#define AGORA_RTM_DATA_MAX_LEN (32 * 1024)
#define AGORA_CREDENTIAL_MAX_LEN 256
#define AGORA_CERTIFICATE_MAX_LEN 1024

/** Error code. */
typedef enum {
  /** No error. */
  ERR_OKAY = 0,

  /** General error */
  ERR_FAILED = 1,

  /**
     * Network is unavailable
     */
  ERR_NET_DOWN = 14,

  /**
     * Request to join channel is rejected.
     * It occurs when local user is already in channel and try to join the same channel again.
     */
  ERR_JOIN_CHANNEL_REJECTED = 17,

  /** App ID is invalid. */
  ERR_INVALID_APP_ID = 101,

  /** Channel is invalid. */
  ERR_INVALID_CHANNEL_NAME = 102,

  /** Fails to get server resources in the specified region. */
  ERR_NO_SERVER_RESOURCES = 103,

  /**
     * Server rejected request to look up channel.
     */
  ERR_LOOKUP_CHANNEL_REJECTED = 105,

  /**
     * Server rejected request to open channel.
     */
  ERR_OPEN_CHANNEL_REJECTED = 107,

  /**
     * Token expired due to reasons belows:
     * - Authorized Timestamp expired:      The timestamp is represented by the number of
     *                                      seconds elapsed since 1/1/1970. The user can use
     *                                      the Token to access the Agora service within five
     *                                      minutes after the Token is generated. If the user
     *                                      does not access the Agora service after five minutes,
     *                                      this Token will no longer be valid.
     * - Call Expiration Timestamp expired: The timestamp indicates the exact time when a
     *                                      user can no longer use the Agora service (for example,
     *                                      when a user is forced to leave an ongoing call).
     *                                      When the value is set for the Call Expiration Timestamp,
     *                                      it does not mean that the Token will be expired,
     *                                      but that the user will be kicked out of the channel.
     */
  ERR_TOKEN_EXPIRED = 109,

  /**
	 * Token is invalid due to reasons belows:
	 * - If application certificate is enabled on the Dashboard,
	 *   valid token SHOULD be set when invoke.
	 *
	 * - If uid field is mandatory, users must specify the same uid as the one used to generate the token,
	 *   when calling `agora_rtc_join_channel`.
	 */
  ERR_INVALID_TOKEN = 110,

  /** Dynamic token has been enabled, but is not provided when joining the channel.
	 *  Please specify the valid token when calling `agora_rtc_join_channel`.
	 */
  ERR_DYNAMIC_TOKEN_BUT_USE_STATIC_KEY = 115,

  /** Switching roles failed.
	 *  Please try to rejoin the channel.
	 */
  ERR_SET_CLIENT_ROLE_NOT_AUTHORIZED = 119,

  /** Decryption fails. The user may have used a different encryption password to join the channel. 
	 *  Check your settings or try rejoining the channel.
	 */
  ERR_DECRYPTION_FAILED = 120,

  /** Ticket to open channel is invalid */
  ERR_OPEN_CHANNEL_INVALID_TICKET = 121,

  /** Try another server. */
  ERR_OPEN_CHANNEL_TRY_NEXT_VOS = 122,

  /** Client is banned by the server */
  ERR_CLIENT_IS_BANNED_BY_SERVER = 123,

  /** Sending video data too fast and over the bandwidth limit.
     *  Very likely that packet loss occurs with this sending speed.
    */
  ERR_SEND_VIDEO_OVER_BANDWIDTH_LIMIT = 200,

  /** Audio decoder does not match incoming audio data type.
     *  Currently SDK built-in audio codec only supports G722 and OPUS.
    */
  ERR_AUDIO_DECODER_NOT_MATCH_AUDIO_FRAME = 201,

  /** Audio decoder does not match incoming audio data type.
     *  Currently SDK built-in audio codec only supports G722 and OPUS.
    */
  ERR_NO_AUDIO_DECODER_TO_HANDLE_AUDIO_FRAME = 202,
} agora_err_code_e;

/**
 * The definition of the user_offline_reason_e enum.
 */
typedef enum {
  /**
     * 0: Remote user leaves channel actively
     */
  USER_OFFLINE_QUIT = 0,
  /**
     * 1: Remote user is dropped due to timeout
     */
  USER_OFFLINE_DROPPED = 1,
} user_offline_reason_e;

/**
 * The definition of log level enum
 */
typedef enum {
  RTC_LOG_DEFAULT = 0,
  RTC_LOG_EMERG,
  RTC_LOG_ALERT,
  RTC_LOG_ERROR,
  RTC_LOG_WARNING,
  RTC_LOG_CRIT, // critical information
  RTC_LOG_NOTICE, // default log level is notice
  RTC_LOG_INFO,
  RTC_LOG_DEBUG,
} rtc_log_level_e;

/** IP areas.
 */
typedef enum {
  /**
	 * Mainland China.
	 */
  AREA_CODE_CN = 0x00000001,
  /**
	 * North America.
	 */
  AREA_CODE_NA = 0x00000002,
  /**
	 * Europe.
	 */
  AREA_CODE_EU = 0x00000004,
  /**
	 * Asia, excluding Mainland China.
	 */
  AREA_CODE_AS = 0x00000008,
  /**
	 * Japan.
	 */
  AREA_CODE_JP = 0x00000010,
  /**
	 * India.
	 */
  AREA_CODE_IN = 0x00000020,
  /**
	 * Oceania
	*/
  AREA_CODE_OC = 0x00000040,
  /**
	 * South-American
	*/
  AREA_CODE_SA = 0x00000080,
  /**
	 * Africa
	*/
  AREA_CODE_AF = 0x00000100,
  /**
	 * South Korea
	 */
  AREA_CODE_KR = 0x00000200,
  /**
	 * The global area (except China)
	 */
  AREA_CODE_OVS = 0xFFFFFFFE,
  /**
	 * (Default) Global.
	 */
  AREA_CODE_GLOB = (0xFFFFFFFF),

} area_code_e;

/**
 * The definition of the video_data_type_e enum.
 */
typedef enum {
  /**
     * 0: YUV420
     */
  VIDEO_DATA_TYPE_YUV420 = 0,
  /**
     * 2: H264
     */
  VIDEO_DATA_TYPE_H264 = 2,
  /**
	 * 3: H265
	 */
  VIDEO_DATA_TYPE_H265 = 3,
  /**
     * 6: generic
     */
  VIDEO_DATA_TYPE_GENERIC = 6,
  /**
     * 20: generic JPEG
     */
  VIDEO_DATA_TYPE_GENERIC_JPEG = 20,
} video_data_type_e;

/**
 * The definition of the video_frame_type_e enum.
 */
typedef enum {
  /**
     * 0: unknow frame type
	 * If you set it ,the SDK will judge the frame type
     */
  VIDEO_FRAME_AUTO_DETECT = 0,
  /**
     * 3: key frame
     */
  VIDEO_FRAME_KEY = 3,
  /*
     * 4: delta frame, e.g: P-Frame
     */
  VIDEO_FRAME_DELTA = 4,
} video_frame_type_e;

/**
 * The definition of the video_frame_rate_e enum.
 */
typedef enum {
  /**
     * 1: 1 fps.
     */
  VIDEO_FRAME_RATE_FPS_1 = 1,
  /**
     * 7: 7 fps.
     */
  VIDEO_FRAME_RATE_FPS_7 = 7,
  /**
     * 10: 10 fps.
     */
  VIDEO_FRAME_RATE_FPS_10 = 10,
  /**
     * 15: 15 fps.
     */
  VIDEO_FRAME_RATE_FPS_15 = 15,
  /**
     * 24: 24 fps.
     */
  VIDEO_FRAME_RATE_FPS_24 = 24,
  /**
     * 30: 30 fps.
     */
  VIDEO_FRAME_RATE_FPS_30 = 30,
  /**
     * 60: 60 fps. Applies to Windows and macOS only.
     */
  VIDEO_FRAME_RATE_FPS_60 = 60,
} video_frame_rate_e;

/**
 * The definition of the video_frame_info_t struct.
 */
typedef struct {
  /**
     * The video data type: #video_data_type_e.
     */
  video_data_type_e data_type;
  /**
     * The frame type of the encoded video frame: #video_frame_type_e.
     */
  video_frame_type_e type;
  /**
     * The number of video frames per second.
     * -This value will be used for calculating timestamps of the encoded image.
     * - If frame_per_sec equals zero, then real timestamp will be used.
     * - Otherwise, timestamp will be adjusted to the value of frame_per_sec set.
     */
  video_frame_rate_e frames_per_sec;
} video_frame_info_t;

/**
 * Audio data type list.
 */
typedef enum {
  /**
     * 1: OPUS
     */
  AUDIO_DATA_TYPE_OPUS = 1,
  /**
   * 3: PCMA
   */
  AUDIO_DATA_TYPE_PCMA = 3,
  /**
   * 4: PCMU
   */
  AUDIO_DATA_TYPE_PCMU = 4,
  /**
     * 5: G722
     */
  AUDIO_DATA_TYPE_G722 = 5,
  /**
     * 8: AACLC
     */
  AUDIO_DATA_TYPE_AACLC = 8,
  /**
     * 9: HEAAC
     */
  AUDIO_DATA_TYPE_HEAAC = 9,
  /**
     * 100: PCM (audio codec should be enabled)
     */
  AUDIO_DATA_TYPE_PCM = 100,
  /**
     * 253: GENERIC
     */
  AUDIO_DATA_TYPE_GENERIC = 253,
} audio_data_type_e;

/**
 * The definition of the audio_frame_info_t struct.
 */
typedef struct {
  /**
     * Audio data type, reference #audio_data_type_e.
     */
  audio_data_type_e data_type;
} audio_frame_info_t;

typedef struct {
  bool log_disable;
  bool log_disable_desensitize;
  rtc_log_level_e log_level;
  const char *log_path;
} log_config_t;

/**
 * The definition of the service option
 */
typedef struct {
  /**
	 * area code for regional restriction, Supported areas refer to enum area_code_e
	 * and bit operations for multiple regions are supported
	 */
  uint32_t area_code;
  /**
	 * the  product_id (device name)
	 * the max product_id_length is 63
	 */
  char *product_id;
  uint32_t product_id_length;
  /**
	 * if set true or not 0, use string uid
	 */
  bool use_string_uid;
  /**
	 * sdk log config
	 */
  log_config_t log_cfg;
} rtc_service_option_t;

/**
 * Audio codec type list.
 */
typedef enum {
  /**
     * 0: OPUS
     */
  AUDIO_CODEC_DISABLED = 0,
  /**
     * 1: OPUS
     */
  AUDIO_CODEC_TYPE_OPUS = 1,
  /**
     * 2: G722
     */
  AUDIO_CODEC_TYPE_G722 = 2,
  /**
     * 3: G711
     */
  AUDIO_CODEC_TYPE_PCMU = 3,
} audio_codec_type_e;

typedef struct {
  /**
     * Configure sdk built-in audio codec
     * If AUDIO_CODEC_TYPE_OPUS is selected, your PCM data is encoded as OPUS and then streamed to Agora channel
     * If AUDIO_CODEC_TYPE_G722 is selected, your PCM data is encoded as G722 and then streamed to Agora channel
     * If you provide encoded audio data, such as AAC, instead of raw PCM, please disable audio codec by selecting AUDIO_CODEC_DISABLED
     */
  audio_codec_type_e audio_codec_type;
  /**
     * Pcm sample rate. Ignored if audio coded is diabled
     */
  int pcm_sample_rate;
  /**
     * Pcm channel number. Ignored if audio coded is diabled
     */
  int pcm_channel_num;
} audio_codec_option_t;

/**
 * The definition of the rtc_channel_options_t struct.
 */
typedef struct {
  bool auto_subscribe_audio;
  bool auto_subscribe_video;
  audio_codec_option_t audio_codec_opt;
  bool enable_encrypt;
} rtc_channel_options_t;

typedef enum {
  CLOUD_PROXY_NONE = 0,
  CLOUD_PROXY_UDP = 1,
  CLOUD_PROXY_TCP = 3,
  CLOUD_PROXY_AUTO = 100,
} cloud_proxy_type_e;

/**
 * Agora RTC SDK event handler
 */
typedef struct {
  /**
      * Occurs when local user joins channel successfully.
      *
      * @param[in] channel    Channel name
      * @param[in] elapsed_ms Time elapsed (ms) since channel is established
      */
  void (*on_join_channel_success)(const char *channel, int elapsed_ms);

  /**
      * Occurs when channel is disconnected from the server.
      *
      * @param[in] channel Channel name
      */
  void (*on_connection_lost)(const char *channel);

  /**
      * Occurs when local user rejoins channel successfully after disconnect
      *
      * When channel loses connection with server due to network problems,
      * SDK will retry to connect automatically. If success, it will be triggered.
      *
      * @param[in] channel    Channel name
      * @param[in] elapsed_ms Time elapsed (ms) since rejoin due to network
      */
  void (*on_rejoin_channel_success)(const char *channel, int elapsed_ms);

  /**
      * Report error message during runtime.
      *
      * In most cases, it means SDK can't fix the issue and application should take action.
      *
     * @param[in] channel Channel name
     * @param[in] code    Error code, see #agora_err_code_e
     * @param[in] msg     Error message
     */
  void (*on_error)(const char *channel, int code, const char *msg);

  /**
     * Occurs when a remote user joins channel successfully.
     *
     * @param[in] channel    Channel name
     * @param[in] uid        Remote user ID
     * @param[in] elapsed_ms Time elapsed (ms) since channel is established
     */
  void (*on_user_joined)(const char *channel, uint32_t uid, int elapsed_ms);

  /**
     * Occurs when remote user leaves the channel.
     *
     * @param[in] channel Channel name
     * @param[in] uid     Remote user ID
     * @param[in] reason  Reason, see #user_offline_reason_e
     */
  void (*on_user_offline)(const char *channel, uint32_t uid, int reason);

  /**
     * Occurs when a remote user sends notification before enable/disable sending audio.
     *
     * @param[in] channel Channel name
     * @param[in] uid     Remote user ID
     * @param[in] muted   Mute status:
     *                    - 0:        unmuted
     *                    - non-ZERO: muted
     */
  void (*on_user_mute_audio)(const char *channel, uint32_t uid, int muted);

  /**
      * Occurs when a remote user sends notification before enable/disable sending video.
      *
     * @param[in] channel Channel name.
     * @param[in] uid     Remote user ID
     * @param[in] muted   Mute status:
     *                    - 0:        unmuted
     *                    - non-ZERO: muted
     */
  void (*on_user_mute_video)(const char *channel, uint32_t uid, int muted);

  /**
      * Occurs when a remote user requests a keyframe.
      *
      * This callback notifies the sender to generate a new keyframe.
      *
      * @param[in] channel    Channel name
      * @param[in] uid        Remote user ID
      * @param[in] stream_id  Stream ID for which a keyframe is requested
      */
  void (*on_key_frame_gen_req)(const char *channel, uint32_t uid, uint8_t stream_id);
  /**
     * Occurs when receiving the audio frame of a remote user in the channel.
     *
     * @param[in] channel    Channel name
     * @param[in] uid        Remote user ID to which data is sent
     * @param[in] sent_ts    Timestamp (ms) for sending data
     * @param[in] data_type  Audio data type
     * @param[in] data_ptr   Audio frame buffer
     * @param[in] data_len   Audio frame buffer length (bytes)
     */
  void (*on_audio_data)(const char *channel, const uint32_t uid, uint16_t sent_ts, audio_data_type_e data_type,
                        const void *data_ptr, size_t data_len);

  /**
     * Occurs every 20ms.
     *
     * @param[in] channel    Channel name
     * @param[in] data_type  Audio data type
     * @param[in] data       Audio frame buffer
     * @param[in] len        Audio frame buffer length (bytes)
     */
  void (*on_mixed_audio_data)(const char *channel, audio_data_type_e data_type, const void *data, size_t len);

  /**
     * Occurs when receiving the video frame of a remote user in the channel.
     *
     * @param[in] channel      Channel name
     * @param[in] uid          Remote user ID to which data is sent
     * @param[in] sent_ts      Timestamp (ms) for sending data
     * @param[in] data_type    Video data type
     * @param[in] stream_id    Video stream ID. Range is [0, 15]
     * @param[in] is_key_frame Frame type:
     *                          - 0:        non-keyframe
     *                          - non-ZERO: keyframe
     * @param[in] data_ptr     Video frame buffer
     * @param[in] data_len     Video frame buffer lenth (bytes)
     */
  void (*on_video_data)(const char *channel, const uint32_t uid, uint16_t sent_ts, video_data_type_e data_type,
                        uint8_t stream_id, int is_key_frame, const void *data_ptr, size_t data_len);

  /**
      * Occurs when network bandwidth change is detected. User is expected to adjust encoder
      * bitrate to |target_bps|
      *
      * @param[in] channel    Channel name
      * @param[in] target_bps Target value (bps) by which the bitrate should update
      */
  void (*on_target_bitrate_changed)(const char *channel, uint32_t target_bps);
  /**
     * Occurs when initialization completes with a given user name.
     *
     * @param[in] uname Local user name provided on initialization
     * @param[in] uid   Local user ID allocated by server
     */
  void (*on_local_user_registered)(const char *uname, uint32_t uid);

  /**
      * Occurs when remote user registered.
      *
      * @param[in] uname Remote user name
      * @param[in] uid   Remote user ID
      */
  void (*on_remote_user_registered)(const char *uname, uint32_t uid);

  /**
	  * Occurs when token will expired.
	  *
	  * @param[in] token The token will expire
	  */
  void (*on_token_privilege_will_expire)(const char *token);

} agora_rtc_event_handler_t;

/** RTM **/

/**
 * RTM event type list
 */
typedef enum {
  /**
	 * 0: LOGIN
	 */
  RTM_EVENT_TYPE_LOGIN = 0,
  /**
	 * 1: KICKOFF
	 */
  RTM_EVENT_TYPE_KICKOFF = 1,
  /**
	 * 2: EXIT
	 */
  RTM_EVENT_TYPE_EXIT = 2,
} rtm_event_type_e;

typedef enum {
  /** no error */
  ERR_RTM_OK = 0,
  /** general error */
  ERR_RTM_FAILED = 1,
  /** Login is rejected by the server. */
  ERR_RTM_LOGIN_REJECTED = 2,
  /**  invalid rtm uid */
  ERR_RTM_INVALID_RTM_UID = 3,
  /** The token is invalid. */
  ERR_RTM_LOGIN_INVALID_TOKEN = 5,
  /** Unauthorized login. */
  ERR_RTM_LOGIN_NOT_AUTHORIZED = 7,
  /**  invalid appid */
  ERR_RTM_INVALID_APP_ID = ERR_INVALID_APP_ID,
  /** The server rejected the request to look up the channel */
  WARN_RTM_LOOKUP_CHANNEL_REJECTED = ERR_LOOKUP_CHANNEL_REJECTED,
  /** Authorized Timestamp expired */
  ERR_RTM_TOKEN_EXPIRED = ERR_TOKEN_EXPIRED,
  /** invalid token */
  ERR_RTM_INVALID_TOKEN = ERR_INVALID_TOKEN,
} rtm_err_code_e;

/**
 * Agora RTC SDK RTM event handler
 */
typedef struct {
  /**
	  * Occurs when data comes from RTM
	  * @param[in] rtm_uid    The remote rtm uid which the data come from.
	  * @param[in] msg        The Data received.
	  * @param[in] msg_len    Length of the data received.
	  */
  void (*on_rtm_data)(const char *rtm_uid, const void *msg, size_t msg_len);
  /**
	  * Occurs when RTM event occurs
	  * @param[in] rtm_uid    RTM UID
	  * @param[in] event_type Event type
	  * @param[in] event_info Event info of event type
	  */
  void (*on_rtm_event)(const char *rtm_uid, rtm_event_type_e event_type, rtm_err_code_e err_code);
  /**
	  * Report the result of the "agora_rtc_send_rtm_data" method call
	  * @param[in] msg_id     Identify one message
	  * @param[in] error_code Error code number
	  *                       - 0 : success
	  *                       - 1 : failure
	  */
  void (*on_send_rtm_data_result)(uint32_t msg_id, rtm_err_code_e error_code);
} agora_rtm_handler_t;
/**
 * Get SDK version.
 *
 * @return A const static string describes the SDK version
 */
extern const char *agora_rtc_get_version(void);

/**
 * Convert error code to const static string.
 *
 * @note You do not have to release the string after use.
 *
 * @param[in] err Error code
 *
 * @return Const static error string
 */
extern __agora_api__ const char *agora_rtc_err_2_str(int err);

/**
 * Generate a credential which is a unique device identifier.
 *
 * @note It's authorizing smart devices license.
 *       You can disregard it if license isn't used.
 *
 * @param[out]    credential        Credential buffer holding the generated data
 * @param[in,out] credential_len    Credential buffer length (bytes), which should be larger than AGORA_CREDENTIAL_MAX_LEN
 *
 * @return
 * - = 0: Success
 * - < 0: Failure
 */
extern __agora_api__ int agora_rtc_license_gen_credential(char *credential, unsigned int *credential_len);

/**
 * Authenticate the SDK licence.
 *
 * @note
 * - It's authorizing smart devices license.
 *   You can disregard it if you do not use a license.
 *   Once the license is enabled, only the authenticated SDK can be used.
 * - This API should be invoked before agora_rtc_init
 *
 * @param[in] certificate     Certificate buffer
 * @param[in] certificate_len Certificate buffer length
 * @param[in] credential      Credential buffer
 * @param[in] credential_len  Credential buffer length
 *
 * @return
 * - = 0: Success
 * - < 0: Failure
 */
extern __agora_api__ int agora_rtc_license_verify(const char *certificate, int certificate_len, const char *credential,
                                                  int credential_len);

/**
 * Initialize the Agora RTSA service.
 *
 * @note Each process can only be initialized once.
 *
 * @param[in] app_id          Application ID
 *                            If 'uid' is set as 0, SDK will assign a valid ID to the user
 * @param[in] event_handler   A set of callback that handles Agora SDK events
 * @param[in] sdk_log_dir     Directory for storing SDK log
 *                            If `sdk_log_dir` is NULL, the log directory is located in:
 *                            - iOS:         Application’s documents directory
 *                            - macOS:       ~/Documents/
 *                            - Android:     /sdcard/
 *                            - Ubuntu:      Application’s current directory
 *                            - Embedded:    Application’s current directory
 * @param[in] option          RTC service option when init, If need't set option, set NULL
 *
 * @return
 * - = 0: Success
 * - < 0: Failure
 */
extern __agora_api__ int agora_rtc_init(const char *app_id, const agora_rtc_event_handler_t *event_handler,
                                        rtc_service_option_t *option);

/**
 * Set the log level.
 *
 * @param[in] level Log level. Range reference to rtc_log_level_e enum
 *
 * @return
 * - = 0: Success
 * - < 0: Failure
 */
extern __agora_api__ int agora_rtc_set_log_level(rtc_log_level_e level);

/**
 * Sets the log file configuration.
 *
 * @param[in] per_logfile_size The size (bytes) of each log file.
 *                             The value range is [0, 10*1024*1024], default 1*1024*1024(1M).
 *                             0 means set log off
 * @param[in] logfile_roll_count The maximum number of log file numbers.
 *                               The value range is [0, 100], default 10.
 *                               0 means set log off
 *
 * @return
 * - 0: Success.
 * - <0: Failure.
 */
extern __agora_api__ int agora_rtc_config_log(int size_per_file, int max_file_count);

/**
 * Local user joins channel.
 *
 * @note Users in the same channel with the same App ID can send data to each other.
 *       You can join more than one channel at the same time. All channels that
 *       you join will receive the audio/video data stream that you send unless
 *       you stop sending the audio/video data stream in a specific channel.
 *
 * @param[in] channel   Channel name
 *                      Length should be less than 64 bytes
 *                      Supported character scopes are:
 *                      - The 26 lowercase English letters: a to z
 *                      - The 26 uppercase English letters: A to Z
 *                      - The 10 numbers: 0 to 9
 *                      - The space
 *                      - "!", "#", "$", "%", "&", "(", ")", "+", "-", ":", ";", "<",
 *                        "=", ".", ">", "?", "@", "[", "]", "^", "_", " {", "}", "|",
 *                        "~", ","
 *
 * @param[in] user_id    User ID, limit length 255. There are two cases:
 *                       case-1: if rtc_service_option_t.use_string_uid == 0 (default):
 *                        (1) set to 32-bit number string, "0" < user_id < "4294967295"
 *                        (2) set to NULL or "" or "0" to auto get uid from sdk
 *                       case-2: if rtc_service_option_t.use_string_uid != 0:
 *                         user_id can be seted to any string such as "agora-user0", then uid will
 *                         be generated by sdk, and on_local_user_registered will be called
 * @param[in] token_ptr Token buffer generated by the server
 *                      - if token authorization is enabled on developer website, it should be set correctly
 *                      - else token can be set as `NULL`
 * @param[in] token_len Token buffer len (bytes), Range is [32, 512]
 *                      If token is set NULL, token_len should be set 0
 * @param[in] options   channel options when create channel.
 *                      If do not set channel options, set NULL
 *
 * @return
 * - = 0: Success
 * - < 0: Failure
 */
extern __agora_api__ int agora_rtc_join_channel(const char *channel, const char *user_id, const void *token_ptr,
                                                size_t token_len, rtc_channel_options_t *options);

/**
 * Allow Local user leaves channel.
 *
 * @note Local user should leave channel when data transmission is stopped
 *
 * @param[in] channel   Channel name
 *                      - NULL for all channels
 *                      - non-NULL for specific channel
 *
 * @return
 * - = 0: Success
 * - < 0: Failure
 */
extern __agora_api__ int agora_rtc_leave_channel(const char *channel);

/**
 * Renew token for specific channel OR all channels.
 *
 * @note Token should be renewed when valid duration reached expiration.
 *
 * @param[in] channel   Channel name. For all channels, set `channel` as NULL
 * @param[in] token_ptr Token buffer
 * @param[in] token_len Token buffer length (bytes). Range is [32, 512]
 *
 * @return
 * - = 0: Success
 * - < 0: Failure
 */
extern __agora_api__ int agora_rtc_renew_token(const char *channel, const void *token_ptr, size_t token_len);

/**
 * Decide whether to enable/disable sending local audio data to specific channel OR all channels.
 *
 * @param[in] channel   Channel name
 *                      - if `channel` is set NULL, it means all channels
 *                      - else it means specific channel
 * @param[in] mute      Toggle sending local audio
 *                      - 0:        enable (default)
 *                      - non-ZERO: disable
 * @return
 * - = 0: Success
 * - < 0: Failure
 */
extern __agora_api__ int agora_rtc_mute_local_audio(const char *channel, int mute);

/**
 * Decide whether to enable/disable sending local video data to specific channel OR all channels.
 *
 * @param[in] channel   Channel name
 *                      - if `channel` is set NULL, it means all channels
 *                      - else it means specific channel
 * @param[in] mute      Toggle sending local video
 *                      - 0:        enable (default)
 *                      - non-ZERO: disable
 *
 * @return
 * - = 0: Success
 * - < 0: Failure
 */
extern __agora_api__ int agora_rtc_mute_local_video(const char *channel, int mute);
/**
 * Decide whether to enable/disable receiving remote audio data from specific channel OR all channels.
 *
 * @param[in] channel       Channel name
 *                          - NULL for all channels
 *                          - non-NULL for specific channel
 * @param[in] remote_uid    Remote user ID
 *                          - if `remote_uid` is set 0, it's for all users
 *                          - else it's for specific user
 * @param[in] mute          Toggle receiving remote audio
 *                          - 0:        enable (default)
 *                          - non-ZERO: disable
 *
 * @return
 * - = 0: Success
 * - < 0: Failure
 */
extern __agora_api__ int agora_rtc_mute_remote_audio(const char *channel, uint32_t remote_uid, int mute);

/**
 * Decide whether to enable/disable receiving remote video data from specific channel OR all channels.
 *
 * @param[in] channel       Channel name
 *                          - NULL for all channels
 *                          - non-NULL for specific channel
 * @param[in] remote_uid    Remote user ID
 *                          - if `remote_uid` is set 0, it's for all users
 *                          - else it's for specific user
 * @param[in] mute          Toggle receiving remote audio
 *                          - 0:        enable (default)
 *                          - non-ZERO: disable
 *
 * @return
 * - = 0: Success
 * - < 0: Failure
 */
extern __agora_api__ int agora_rtc_mute_remote_video(const char *channel, uint32_t remote_uid, int mute);

/**
 * Request remote user to generate a keyframe for all video streams OR specific video stream.
 *
 * @param[in] channel      Channel name
 *                         - NULL for all channels
 *                         - non-NULL for specific channel
 * @param[in] remote_uid   Remote user ID
 *                         - if `remote_uid` is set 0, it's for all users
 *                         - else it's for specific user
 * @param[in] stream_id    Stream ID
 *                         - if `stream_id` is set 0, it's for all video streams
 *                         - else it's for specific video stream
 *
 * @return
 * - = 0: Success
 * - < 0: Failure
 */
extern __agora_api__ int agora_rtc_request_video_key_frame(const char *channel, uint32_t remote_uid, uint8_t stream_id);
/**
 * Send an audio frame to all channels OR specific channel.
 *
 * All remote users in this channel will receive the audio frame.
 *
 * @note All channels that you joined will receive the audio frame that you send
 *       unless you stop sending the local audio to a specific channel.
 *
 * @param[in] channel   Channel name
 *                      - NULL for all channels
 *                      - non-NULL for specific channel
 * @param[in] data_ptr  Audio frame buffer
 * @param[in] data_len  Audio frame buffer length (bytes)
 * @param[in] info_ptr  Audio frame info, see #audio_frame_info_t
 *
 * @return
 * - = 0: Success
 * - < 0: Failure
 */
extern __agora_api__ int agora_rtc_send_audio_data(const char *channel, const void *data_ptr, size_t data_len,
                                                   audio_frame_info_t *info_ptr);

/**
 * Send a video frame to all channels OR specific channel.
 *
 * All remote users in the channel will receive the video frame.
 *
 * @note All channels that you join will receive the video frame that you send
 *       unless you stop sending the local video to a specific channel.
 *
 * @param[in] channel   Channel name
 *                      - NULL     for all channels
 *                      - non-NULL for specific channel
 * @param[in] stream_id Stream ID
 * @param[in] data_ptr  Video frame buffer
 * @param[in] data_len  Video frame buffer length (bytes)
 * @param[in] info_ptr  Video frame info
 *
 * @return
 * - = 0: Success
 * - < 0: Failure
 */
extern __agora_api__ int agora_rtc_send_video_data(const char *channel, uint8_t stream_id, const void *data_ptr,
                                                   size_t data_len, video_frame_info_t *info_ptr);

/**
 * Login agora RTM service
 * @param[in] rtm_uid   RTM user id (different from uid)
 *                      Length should be less than 64 bytes
 *                      Supported character scopes are:
 *                      - The 26 lowercase English letters: a to z
 *                      - The 26 uppercase English letters: A to Z
 *                      - The 10 numbers: 0 to 9
 *                      - The space
 *                      - "!", "#", "$", "%", "&", "(", ")", "+", "-", ":",
 *                        ";", "<", "=", ".", ">", "?", "@", "[", "]", "^",
 *                        "_", " {", "}", "|", "~", ","
* @param[in] rtm_token  RTM token string generated by the token server (different from RTC token)
 *                      - if token authorization is enabled on developer website, it should be set correctly
 *                      - else token can be set as `NULL`
 * @param[in] hanlder   A set of callback that handles Agora RTM events
 *
 * @return
 * - = 0: Success
 * - < 0: Failure
 */
extern __agora_api__ int agora_rtc_login_rtm(const char *rtm_uid, const char *rtm_token,
                                             const agora_rtm_handler_t *handler);

/**
 * Logout agora RTM service
 *
 * @return
 * - = 0: Success
 * - < 0: Failure
 */

extern __agora_api__ int agora_rtc_logout_rtm(void);

/**
 * Send data through Real-time Messaging (RTM) mechanism, which is a stable and reliable data channel
 * @note RTM channel is not available by default, unless login success and callback `on_rtm_event`
         is triggered. The sending speed allowed is limited to 60 messages per second (60qps)
 *
 * @param[in] rtm_uid     RTM UID
 * @param[in] msg_id  Identify the message sent
 * @param[in] msg     Message to send
 * @param[in] msg_len Length of the message(max size: 32KB)
 *
 * @return:
 * - = 0: Success
 * - < 0: Failure
 */
extern __agora_api__ int agora_rtc_send_rtm_data(const char *rtm_uid, uint32_t msg_id, const void *msg, size_t msg_len);

/**
 * Set network bandwdith estimation (bwe) param
 *
 * @param[in] min_bps   : bwe min bps
 * @param[in] max_bps   : bwe max bps
 * @param[in] start_bps : bwe start bps
 *
 * @return:
 * - = 0: Success
 * - < 0: Failure
 */
extern __agora_api__ int agora_rtc_set_bwe_param(uint32_t min_bps, uint32_t max_bps, uint32_t start_bps);

#ifdef CONFIG_RTC_PROXY
/**
 * Set cloud proxy
 * @param [in] type : reference enum cloud_proxy_type_e
 * @return 0: Success, <0: failure
 */
extern __agora_api__ int agora_rtc_set_cloud_proxy(cloud_proxy_type_e type);
#endif // End CONFIG_RTC_PROXY

/**
 * Set config params
 *
 * @param [in] params : config params described by json
 * @note  supported sets are shown below, they can be together in params json string
 *  - {"rtc.encryption": {"enable": true/false, "master_key": "xxx..."}}
 *  - {"rtc_network": { "type": INT_xx, "id": INT_xx, "update": true|false }}
 * @return:
 *  - = 0: Success
 *  - < 0: Failure
 */
extern __agora_api__ int agora_rtc_set_params(const char *params);

/**
 * Release all resource allocated by Agora RTSA SDK
 *
 * @return
 * - = 0: Success
 * - < 0: Failure
 */
extern __agora_api__ int agora_rtc_fini(void);

#ifdef __cplusplus
}
#endif

#endif /* __AGORA_RTC_API_H__ */
