//
// Created by CainHuang on 2020-02-24.
//

#include "CAVMediaPlayer.h"

CAVMediaPlayer::CAVMediaPlayer() {
    LOGD("CAVMediaPlayer::constructor()");
    mThread = nullptr;
    mMessageQueue = std::unique_ptr<MessageQueue>(new MessageQueue());
    mTimestamp = std::make_shared<Timestamp>();
    mStreamPlayListener = std::make_shared<MediaPlayerListener>(this);
    mVideoPlayer = std::make_shared<VideoStreamGLPlayer>(mStreamPlayListener);
    mVideoPlayer->setTimestamp(mTimestamp);

    mAudioPlayer = std::make_shared<AudioStreamPlayer>(mStreamPlayListener);
    mAudioPlayer->setTimestamp(mTimestamp);
    mPlayListener = nullptr;
    mAbortRequest = true;
}

CAVMediaPlayer::~CAVMediaPlayer() {
    release();
    LOGD("CAVMediaPlayer::destructor()");
}

void CAVMediaPlayer::init() {
    mAbortRequest = false;
    mCondition.signal();
    if (mThread == nullptr) {
        mThread = new Thread(this);
    }
    if (!mThread->isActive()) {
        mThread->start();
    }
}

void CAVMediaPlayer::release() {
    mAbortRequest = true;
    mCondition.signal();
    if (mThread != nullptr) {
        mThread->join();
        delete mThread;
        mThread = nullptr;
    }
    if (mPlayListener != nullptr) {
        mPlayListener.reset();
        mPlayListener = nullptr;
    }
    if (mStreamPlayListener != nullptr) {
        mStreamPlayListener.reset();
        mStreamPlayListener = nullptr;
    }
    if (mAudioPlayer != nullptr) {
        mAudioPlayer->release();
        mAudioPlayer.reset();
        mAudioPlayer = nullptr;
    }
    if (mVideoPlayer != nullptr) {
        mVideoPlayer->release();
        mVideoPlayer.reset();
        mVideoPlayer = nullptr;
    }
    if (mMessageQueue != nullptr) {
        mMessageQueue->flush();
        mMessageQueue.reset();
        mMessageQueue = nullptr;
    }
    if (mTimestamp != nullptr) {
        mTimestamp.reset();
        mTimestamp = nullptr;
    }
}

void CAVMediaPlayer::setOnPlayingListener(std::shared_ptr<OnPlayListener> listener) {
    if (mPlayListener != nullptr) {
        mPlayListener.reset();
        mPlayListener = nullptr;
    }
    mPlayListener = listener;
}

status_t CAVMediaPlayer::setDataSource(const char *url, int64_t offset, const char *headers) {
    LOGD("CAVMediaPlayer::setDataSource(): %s, offset: %d, headers: %s", url, offset, headers);
    if (mAudioPlayer != nullptr) {
        mAudioPlayer->setDataSource(url);
    }
    if (mVideoPlayer != nullptr) {
        mVideoPlayer->setDataSource(url);
    }
    return OK;
}

status_t CAVMediaPlayer::setAudioDecoder(const char *decoder) {
    LOGD("CAVMediaPlayer::setAudioDecoder(): %s", decoder);
    if (mAudioPlayer != nullptr) {
        mAudioPlayer->setDecoderName(decoder);
    }
    return OK;
}

status_t CAVMediaPlayer::setVideoDecoder(const char *decoder) {
    LOGD("CAVMediaPlayer::setVideoDecoder(): %s", decoder);
    if (mVideoPlayer != nullptr) {
        mVideoPlayer->setDecoderName(decoder);
    }
    return OK;
}

#if defined(__ANDROID__)
status_t CAVMediaPlayer::setVideoSurface(ANativeWindow *window) {
    LOGD("CAVMediaPlayer::setVideoSurface()");
    if (mVideoPlayer != nullptr) {
        mVideoPlayer->surfaceCreated(window);
    }
    return OK;
}

status_t CAVMediaPlayer::changeSurface(int width, int height) {
    LOGD("CAVMediaPlayer::changeSurface()");
    if (mVideoPlayer != nullptr) {
        mVideoPlayer->surfaceChange(width, height);
    }
    return OK;
}

#endif

status_t CAVMediaPlayer::setAutoAspectFit(bool autoFit) {
    if (mVideoPlayer != nullptr) {
        mVideoPlayer->setAutoAspectFit(autoFit);
    } else {
    }
    return OK;
}

status_t CAVMediaPlayer::setSpeed(float speed) {
    LOGD("CAVMediaPlayer::setSpeed(): %.2f", speed);
    if (mAudioPlayer != nullptr) {
        mAudioPlayer->setSpeed(speed);
    }
    if (mVideoPlayer != nullptr) {
        mVideoPlayer->setSpeed(speed);
    }
    return OK;
}

status_t CAVMediaPlayer::setLooping(bool looping) {
    LOGD("CAVMediaPlayer::setLooping(): %d", looping);
    if (mAudioPlayer != nullptr) {
        mAudioPlayer->setLooping(looping);
    }
    if (mVideoPlayer != nullptr) {
        mVideoPlayer->setLooping(looping);
    }
    return OK;
}

status_t CAVMediaPlayer::setRange(float start, float end) {
    LOGD("CAVMediaPlayer::setRange(): {%.2f, %.2f}", start, end);
    if (mAudioPlayer != nullptr) {
        mAudioPlayer->setRange(start, end);
    }
    if (mVideoPlayer != nullptr) {
        mVideoPlayer->setRange(start, end);
    }
    return OK;
}

status_t CAVMediaPlayer::setVolume(float leftVolume, float rightVolume) {
    LOGD("CAVMediaPlayer::setVolume(): {%.2f, %.2f}", leftVolume, rightVolume);
    if (mAudioPlayer != nullptr) {
        mAudioPlayer->setVolume(leftVolume, rightVolume);
    }
    return OK;
}

status_t CAVMediaPlayer::setMute(bool mute) {
    return OK;
}

status_t CAVMediaPlayer::prepare() {
    LOGD("CAVMediaPlayer::prepare()");
    mMessageQueue->pushMessage(new Message(MSG_REQUEST_PREPARE));
    mCondition.signal();
    return OK;
}

status_t CAVMediaPlayer::start() {
    LOGD("CAVMediaPlayer::start()");
    mMessageQueue->pushMessage(new Message(MSG_REQUEST_START));
    mCondition.signal();
    return OK;
}

status_t CAVMediaPlayer::pause() {
    LOGD("CAVMediaPlayer::pause()");
    mMessageQueue->pushMessage(new Message(MSG_REQUEST_PAUSE));
    mCondition.signal();
    return OK;
}

status_t CAVMediaPlayer::stop() {
    LOGD("CAVMediaPlayer::stop()");
    mMessageQueue->pushMessage(new Message(MSG_REQUEST_STOP));
    mCondition.signal();
    return OK;
}

status_t CAVMediaPlayer::setDecodeOnPause(bool decodeOnPause) {
    if (mVideoPlayer != nullptr) {
        mVideoPlayer->setDecodeOnPause(decodeOnPause);
    }
    return OK;
}

status_t CAVMediaPlayer::seekTo(float timeMs) {
    mMessageQueue->pushMessage(new Message(MSG_REQUEST_SEEK, (int)(timeMs * 1000), -1));
    mCondition.signal();
    return OK;
}

long CAVMediaPlayer::getCurrentPosition() {
    return 0;
}

float CAVMediaPlayer::getDuration() {
    float duration = 0;
    if (mAudioPlayer != nullptr) {
        duration = mAudioPlayer->getDuration();
    }
    if (mVideoPlayer != nullptr && duration < mVideoPlayer->getDuration()) {
        duration = mVideoPlayer->getDuration();
    }
    return duration;
}

int CAVMediaPlayer::getRotate() {
    if (mVideoPlayer != nullptr) {
        return mVideoPlayer->getRotate();
    }
    return 0;
}

int CAVMediaPlayer::getVideoWidth() {
    if (mVideoPlayer != nullptr) {
        return mVideoPlayer->getVideoWidth();
    }
    return 0;
}

int CAVMediaPlayer::getVideoHeight() {
    if (mVideoPlayer != nullptr) {
        return mVideoPlayer->getVideoHeight();
    }
    return 0;
}

bool CAVMediaPlayer::isLooping() {
    bool loop = false;
    if (mAudioPlayer != nullptr) {
        loop |= mAudioPlayer->isLooping();
    }
    if (mVideoPlayer != nullptr) {
        loop |= mVideoPlayer->isLooping();
    }
    return loop;
}

bool CAVMediaPlayer::isPlaying() {
    bool playing = false;
    if (mAudioPlayer != nullptr) {
        playing |= mAudioPlayer->isPlaying();
    }
    if (mVideoPlayer != nullptr) {
        playing |= mVideoPlayer->isPlaying();
    }
    return playing;
}

bool CAVMediaPlayer::hasAudio() {
    if (mAudioPlayer != nullptr) {
        return mAudioPlayer->hasAudio();
    }
    return false;
}

bool CAVMediaPlayer::hasVideo() {
    if (mVideoPlayer != nullptr) {
        return mVideoPlayer->hasVideo();
    }
    return false;
}

void CAVMediaPlayer::onPlaying(float pts) {
    mMessageQueue->pushMessage(new Message(MSG_CURRENT_POSITION, pts, getDuration()));
    mCondition.signal();
    if (hasVideo()) {
        notify(MSG_VIDEO_CURRENT_POSITION, mTimestamp->getVideoClock(), getDuration());
    }
}

void CAVMediaPlayer::onSeekComplete(AVMediaType type) {
    if (type == AVMEDIA_TYPE_VIDEO && hasVideo()) {
        mMessageQueue->pushMessage(new Message(MSG_SEEK_COMPLETE));
        mCondition.signal();
    }
}

void CAVMediaPlayer::onCompletion(AVMediaType type) {
    mMessageQueue->pushMessage(new Message(MSG_COMPLETED));
    mCondition.signal();
}

void CAVMediaPlayer::changeFilter(int type, const char *name) {
    if (mVideoPlayer != nullptr) {
        mVideoPlayer->changeFilter((RenderNodeType)type, name);
    }
}

void CAVMediaPlayer::changeFilter(int type, const int id) {
    if (mVideoPlayer != nullptr) {
        mVideoPlayer->changeFilter((RenderNodeType)type, id);
    }
}

void CAVMediaPlayer::notify(int msg, int arg1, int arg2) {
    mMessageQueue->pushMessage(new Message(msg, arg1, arg2));
    mCondition.signal();
}

void CAVMediaPlayer::postEvent(int what, int arg1, int arg2, void *obj) {
    if (mPlayListener != nullptr) {
        mPlayListener->notify(what, arg1, arg2, obj);
    }
}

void CAVMediaPlayer::run() {
    while (true) {

        if (mAbortRequest) {
            break;
        }

        mMutex.lock();
        if (mMessageQueue->empty()) {
            mCondition.wait(mMutex);
        }
        mMutex.unlock();

        auto msg = mMessageQueue->popMessage();
        if (!msg) {
            continue;
        }
        int what = msg->getWhat();
        switch(what) {

            // 刷新缓冲区
            case MSG_FLUSH: {
                LOGD("MediaPlayer is flushing.\n");
                break;
            }

            // 播放出错
            case MSG_ERROR: {
                LOGD("MediaPlayer occurs error: %d\n", msg->getArg1());
                postEvent(MEDIA_ERROR, msg->getArg1(), 0);
                break;
            }

            // 准备完成回调
            case MSG_PREPARED: {
                LOGD("MediaPlayer is prepared.\n");
                postEvent(MEDIA_PREPARED);
                break;
            }

            // 开始播放回调
            case MSG_STARTED: {
                LOGD("MediaPlayer is started!");
                postEvent(MEDIA_STARTED);
                break;
            }

            // 播放完成回调
            case MSG_COMPLETED: {
                LOGD("MediaPlayer is playback completed.\n");
                postEvent(MEDIA_PLAYBACK_COMPLETE);
                break;
            }

            // 视频大小发生变化回调
            case MSG_VIDEO_SIZE_CHANGED: {
                LOGD("MediaPlayer is video size changing: %d, %d\n", msg->getArg1(), msg->getArg2());
                postEvent(MEDIA_SET_VIDEO_SIZE, msg->getArg1(), msg->getArg2());
                break;
            }

            // sar发生变化回调
            case MSG_SAR_CHANGED: {
                LOGD("MediaPlayer is sar changing: %d, %d\n", msg->getArg1(), msg->getArg2());
                postEvent(MEDIA_SET_VIDEO_SAR, msg->getArg1(), msg->getArg2());
                break;
            }

            // 视频开始渲染回调
            case MSG_VIDEO_RENDERING_START: {
                LOGD("MediaPlayer is video playing.\n");
                break;
            }

            // 音频开始播放回调
            case MSG_AUDIO_RENDERING_START: {
                LOGD("MediaPlayer is audio playing.\n");
                break;
            }

            // 视频旋转改变回调
            case MSG_VIDEO_ROTATION_CHANGED: {
                LOGD("MediaPlayer's video rotation is changing: %d\n", msg->getArg1());
                break;
            }

            // 打开音频解码器回调
            case MSG_AUDIO_START: {
                LOGD("MediaPlayer starts audio decoder.\n");
                break;
            }

            // 打开解码器回调
            case MSG_VIDEO_START: {
                LOGD("MediaPlayer starts video decoder.\n");
                break;
            }

            // 打开输入文件回调
            case MSG_OPEN_INPUT: {
                LOGD("MediaPlayer is opening input file.\n");
                break;
            }

            // 查找媒体流信息回调
            case MSG_FIND_STREAM_INFO: {
                LOGD("CanMediaPlayer is finding media stream info.\n");
                break;
            }

            // 准备解码器回调
            case MSG_PREPARE_DECODER: {
                LOGD("MediaPlayer is preparing decoder.\n");
                break;
            }

            // 开始缓冲回调
            case MSG_BUFFERING_START: {
                LOGD("CanMediaPlayer is buffering start.\n");
                postEvent(MEDIA_INFO, MEDIA_INFO_BUFFERING_START, msg->getArg1());
                break;
            }

            // 缓冲结束回调
            case MSG_BUFFERING_END: {
                LOGD("MediaPlayer is buffering finish.\n");
                postEvent(MEDIA_INFO, MEDIA_INFO_BUFFERING_END, msg->getArg1());
                break;
            }

            // 缓冲区更新回调
            case MSG_BUFFERING_UPDATE: {
                LOGD("MediaPlayer is buffering: %d, %d", msg->getArg1(), msg->getArg2());
                postEvent(MEDIA_BUFFERING_UPDATE, msg->getArg1(), msg->getArg2());
                break;
            }

            // 跳转完成回调
            case MSG_SEEK_COMPLETE: {
                LOGD("MediaPlayer seeks completed!\n");
                postEvent(MEDIA_SEEK_COMPLETE);
                break;
            }

            // 准备操作
            case MSG_REQUEST_PREPARE: {
                preparePlayer();
                break;
            }

            // 开始
            case MSG_REQUEST_START: {
                startPlayer();
                break;
            }

            // 暂停
            case MSG_REQUEST_PAUSE: {
                pausePlayer();
                break;
            }

            // 停止
            case MSG_REQUEST_STOP: {
                stopPlayer();
                break;
            }

            // 定位
            case MSG_REQUEST_SEEK: {
                float timeMs = msg->getArg1() / 1000.0f;
                seekPlayer(timeMs);
                break;
            }

            // 播放进度回调
            case MSG_CURRENT_POSITION: {
                postEvent(MEDIA_CURRENT, msg->getArg1(), msg->getArg2());
                break;
            }

            // 视频流播放进度回调
            case MSG_VIDEO_CURRENT_POSITION: {
                postEvent(MEDIA_VIDEO_CURRENT, msg->getArg1(), msg->getArg2());
                break;
            }

            default: {
                LOGE("MediaPlayer unknown MSG_xxx(%d)\n", msg->getWhat());
                break;
            }
        }
        delete msg;
    }
}

/**
 * 准备回调
 */
void CAVMediaPlayer::preparePlayer() {
    if (mVideoPlayer != nullptr) {
        mVideoPlayer->prepare();
    }
    if (mAudioPlayer != nullptr) {
        mAudioPlayer->prepare();
    }
    // 准备完成消息
    mMessageQueue->pushMessage(new Message(MSG_PREPARED));
    // 视频大小发生变化的消息
    if (mVideoPlayer != nullptr && mVideoPlayer->hasVideo()) {
        mMessageQueue->pushMessage(new Message(MSG_VIDEO_SIZE_CHANGED, mVideoPlayer->getVideoWidth(), mVideoPlayer->getVideoHeight()));
    }
    mCondition.signal();
}

/**
 * 开始播放
 */
void CAVMediaPlayer::startPlayer() {
    if (mVideoPlayer != nullptr) {
        mVideoPlayer->start();
    }
    if (mAudioPlayer != nullptr) {
        mAudioPlayer->start();
    }
    LOGD("start success");
}

/**
 * 暂停播放器
 */
void CAVMediaPlayer::pausePlayer() {
    if (mVideoPlayer != nullptr) {
        mVideoPlayer->pause();
    }
    if (mAudioPlayer != nullptr) {
        mAudioPlayer->pause();
    }
    LOGD("pause finish");
}

/**
 * 停止播放器
 */
void CAVMediaPlayer::stopPlayer() {
    if (mAudioPlayer != nullptr) {
        mAudioPlayer->stop();
    }
    if (mVideoPlayer != nullptr) {
        mVideoPlayer->stop();
    }
}

/**
 * 跳转到某个时间点
 * @param timeMs    跳转时间(ms)
 */
void CAVMediaPlayer::seekPlayer(float timeMs) {
    if (mAudioPlayer != nullptr) {
        mAudioPlayer->seekTo(timeMs);
    }
    if (mVideoPlayer != nullptr) {
        mVideoPlayer->seekTo(timeMs);
    }
}

MediaPlayerListener::MediaPlayerListener(CAVMediaPlayer *player) {
    this->player = player;
}

MediaPlayerListener::~MediaPlayerListener() {
    this->player = nullptr;
    LOGD("MediaPlayerListener::destructor()");
}

void MediaPlayerListener::onPrepared(AVMediaType type) {

}

void MediaPlayerListener::onPlaying(AVMediaType type, float pts) {
    if (!player) {
        return;
    }
    // 存在视频流，则优先使用视频流的时间戳
    if (player->hasVideo() && type == AVMEDIA_TYPE_VIDEO) {
        player->onPlaying(pts);
    } else if (player->hasAudio() && type == AVMEDIA_TYPE_AUDIO) {
        player->onPlaying(pts);
    }
}

void MediaPlayerListener::onSeekComplete(AVMediaType type) {
    if (player != nullptr ) {
        player->onSeekComplete(type);
    }
}

void MediaPlayerListener::onCompletion(AVMediaType type) {
    if (player != nullptr ) {
        player->onCompletion(type);
    }
}

void MediaPlayerListener::onError(AVMediaType type, int errorCode, const char *msg) {

}
