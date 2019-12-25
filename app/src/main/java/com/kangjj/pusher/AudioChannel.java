package com.kangjj.pusher;

import android.media.AudioFormat;
import android.media.AudioRecord;
import android.media.MediaRecorder;

import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;
//todo B 音频推流
public class AudioChannel {
    private NEPusher mPusher;
    private boolean isLive;
    private int channels = 2;
    private AudioRecord audioRecord;
    private ExecutorService executorService;
    int inputSamples;       //字节数
    public AudioChannel(NEPusher pusher) {
        this.mPusher = pusher;
        executorService = Executors.newSingleThreadExecutor();
        int channelConfig;
        if(channels == 2 ){
            channelConfig = AudioFormat.CHANNEL_IN_STEREO;
        }else{
            channelConfig = AudioFormat.CHANNEL_IN_MONO;
        }
        //todo B.1 初始化faac音频编码器
        mPusher.native_initAudioEncoder(44100,channels);
        //todo B.2 编码器一次最大能接受的样本数（一个样本16bit，2字节）
        inputSamples = mPusher.getInputSames()*2;   //16bit的样本数 = 2字节

        int minBufferSize = AudioRecord.getMinBufferSize(
                44100,channelConfig,AudioFormat.ENCODING_PCM_16BIT)*2;
        //todo B.3 实例化AudioRecord
        audioRecord = new AudioRecord(MediaRecorder.AudioSource.MIC,44100,
                channelConfig,AudioFormat.ENCODING_PCM_16BIT,Math.max(inputSamples,minBufferSize));
    }

    public void startLive() {
        isLive = true;
        executorService.submit(new AudioTask());
    }

    public void stopLive() {
        isLive =false;
    }

    public void release() {
        if(audioRecord!=null){
            audioRecord.release();
            audioRecord = null;
        }
    }

    private class AudioTask implements Runnable{

        @Override
        public void run() {
            //todo B.4 在线程里面进行数据的采集
            audioRecord.startRecording();//开始录音
            byte[] bytes = new byte[inputSamples];
            while (isLive) {
                //每次读多少数据要根据编码器来定
                int len =audioRecord.read(bytes,0,bytes.length);
                if(len > 0){
                    //todo B.4 成功采集到的数据了
                    // 对音频数据进行编码并发送（将编码后 数据push到安全队列中）
                    mPusher.native_pushAudio(bytes);
                }
            }
            audioRecord.stop();
        }
    }
}
