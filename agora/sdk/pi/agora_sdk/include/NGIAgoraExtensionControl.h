//
// Copyright (c) 2020 Agora.io. All rights reserved

// This program is confidential and proprietary to Agora.io.
// And may not be copied, reproduced, modified, disclosed to others, published
// or used, in whole or in part, without the express prior written permission
// of Agora.io.

#pragma once  // NOLINT(build/header_guard)

#include "AgoraBase.h"
#include "AgoraRefPtr.h"
#include "IAgoraLog.h"
#include "NGIAgoraVideoFrame.h"
#include "NGIAgoraMediaNodeFactory.h"

namespace agora {
namespace rtc {

/**
 * Interface for extension providers.
 *
 * You can implement these interfaces to implement your extensions to SDK.
 * See \ref agora::RefCountedObject to wrap your implementation so that it can be
 * held by agora::agora_refptr.
 * For example:
 * ```cpp
 * class YourExtensionProvider: public IExtensionProvider {
 *  // Your implementation
 * };
 *
 * // Use agora::RefCountedObject to provide RefCountInterface implementation for you implementation,
 * // intantiate and wrap it with agora_refptr.
 *
 * agora_refptr<IExtensionProvider> provider = new RefCountedObject<YourExtensionProvide>(Arg1, Arg2, ...);
 *```
 *
 * You can instantiate your AudioFilter, VideoFilter, or VideoSink in your own extension provider.
 */
class IExtensionProvider : public RefCountInterface {
 public:
  virtual agora_refptr<IAudioFilter> createAudioFilter(const char* name) = 0;
  virtual agora_refptr<IVideoFilter> createVideoFilter(const char* name) = 0;
  virtual agora_refptr<IVideoSinkBase> createVideoSink(const char* name) = 0;

 protected:
  ~IExtensionProvider() {}
};

/**
 * Interface for handling agora extensions.
 */
class IExtensionControl {
 public:
  /**
   * Agora Extension Capabilities.
   */
  struct Capabilities {
    /**
     * Whether to support audio extensions.
     */
    bool audio;
    /**
     * Whether to support video extensions.
     */
    bool video;
  };

  /**
   * Gets the capabilities of agora extensions.
   *
   * @param capabilities Supported extension capabilities.
   */
  virtual void getCapabilities(Capabilities& capabilities) = 0;

  /**
   * Registers an extension provider to the SDK.
   *
   * @param vendor_name Name of the vendor that identifies the provider.
   * @param provider Extension provider implemented by the vendor.
   * @return
   * - 0: The method call succeeds.
   * - <0: The method call fails.
   */
  virtual int registerExtensionProvider(
    const char* vendor_name, agora::agora_refptr<agora::rtc::IExtensionProvider> provider) = 0;

  /**
   * Unregisters the extension provider from the SDK.
   *
   * @param vendor_name Name of the vendor that identifies the provider.
   * @return
   * - 0: The method call succeeds.
   * - <0: The method call fails.
   */
  virtual int unregisterExtensionProvider(const char* vendor_name) = 0;

  /**
   * Creates an IVideoFrame object with a specified video frame type, format, width and height.
   *
   * @return
   * - The pointer to {@link IVideoFrame}: The method call succeeds.
   * - An empty pointer nullptr: The method call fails.
   */
  virtual agora_refptr<IVideoFrame> createVideoFrame(
      IVideoFrame::Type type, IVideoFrame::Format format, int width, int height) = 0;

  /**
   * Creates a new IVideoFrame object by copying from the source video frame.
   *
   * @return
   * - The pointer to {@link IVideoFrame}: The method call succeeds.
   * - An empty pointer nullptr: The method call fails.
   */
  virtual agora_refptr<IVideoFrame> copyVideoFrame(agora_refptr<IVideoFrame> src) = 0;

  /**
   * Recycles internal frame memory with a specified Video frame type.
   *
   * The SDK automatically recycles deprecated video frames. However,
   * you can still call this method to perform an immediate memory recycle.
   * @param type Frame type to be recycled.
   */
  virtual void recycleVideoCache(IVideoFrame::Type type) = 0;

  /**
   * This method dumps the content of the video frame to the specified file.
   *
   * @return
   * - 0: The method call succeeds.
   * - <0: The method call fails.
   */
  virtual int dumpVideoFrame(agora_refptr<IVideoFrame> frame, const char* file) = 0;

  /**
   * Sets log file.
   *
   * @param level Logging level. See #commons::LOG_LEVEL.
   * @param message Message to add to the log file.
   * @return
   * - 0: The method call succeeds.
   * - <0: The method call fails.
   */
  virtual int log(commons::LOG_LEVEL level, const char* message) = 0;

 protected:
  virtual ~IExtensionControl() {}
};

}  // namespace rtc
}  // namespace agora
