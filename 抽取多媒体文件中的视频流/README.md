# 抽取多媒体文件中的视频流

## 关键API

1. `avformat_open_input`

   ```c
   /**
    * Open an input stream and read the header. The codecs are not opened.
    * The stream must be closed with avformat_close_input().
    *
    * @param ps Pointer to user-supplied AVFormatContext (allocated by avformat_alloc_context).
    *           May be a pointer to NULL, in which case an AVFormatContext is allocated by this
    *           function and written into ps.
    *           Note that a user-supplied AVFormatContext will be freed on failure.
    * @param url URL of the stream to open.
    * @param fmt If non-NULL, this parameter forces a specific input format.
    *            Otherwise the format is autodetected.
    * @param options  A dictionary filled with AVFormatContext and demuxer-private options.
    *                 On return this parameter will be destroyed and replaced with a dict containing
    *                 options that were not found. May be NULL.
    *
    * @return 0 on success, a negative AVERROR on failure.
    *
    * @note If you want to use custom IO, preallocate the format context and set its pb field.
    */
   int avformat_open_input(AVFormatContext **ps, const char *url, AVInputFormat *fmt, AVDictionary **options);
   ```

2. `av_find_best_stream`

   ```c
   /**
    * Find the "best" stream in the file.
    * The best stream is determined according to various heuristics as the most
    * likely to be what the user expects.
    * If the decoder parameter is non-NULL, av_find_best_stream will find the
    * default decoder for the stream's codec; streams for which no decoder can
    * be found are ignored.
    *
    * @param ic                media file handle
    * @param type              stream type: video, audio, subtitles, etc.
    * @param wanted_stream_nb  user-requested stream number,
    *                          or -1 for automatic selection
    * @param related_stream    try to find a stream related (eg. in the same
    *                          program) to this one, or -1 if none
    * @param decoder_ret       if non-NULL, returns the decoder for the
    *                          selected stream
    * @param flags             flags; none are currently defined
    * @return  the non-negative stream number in case of success,
    *          AVERROR_STREAM_NOT_FOUND if no stream with the requested type
    *          could be found,
    *          AVERROR_DECODER_NOT_FOUND if streams were found but no decoder
    * @note  If av_find_best_stream returns successfully and decoder_ret is not
    *        NULL, then *decoder_ret is guaranteed to be set to a valid AVCodec.
    */
   int av_find_best_stream(AVFormatContext *ic,
                           enum AVMediaType type,
                           int wanted_stream_nb,
                           int related_stream,
                           AVCodec **decoder_ret,
                           int flags);
   ```

3. `av_read_frame`

   ```c
   /**
    * Return the next frame of a stream.
    * This function returns what is stored in the file, and does not validate
    * that what is there are valid frames for the decoder. It will split what is
    * stored in the file into frames and return one for each call. It will not
    * omit invalid data between valid frames so as to give the decoder the maximum
    * information possible for decoding.
    *
    * If pkt->buf is NULL, then the packet is valid until the next
    * av_read_frame() or until avformat_close_input(). Otherwise the packet
    * is valid indefinitely. In both cases the packet must be freed with
    * av_packet_unref when it is no longer needed. For video, the packet contains
    * exactly one frame. For audio, it contains an integer number of frames if each
    * frame has a known fixed size (e.g. PCM or ADPCM data). If the audio frames
    * have a variable size (e.g. MPEG audio), then it contains one frame.
    *
    * pkt->pts, pkt->dts and pkt->duration are always set to correct
    * values in AVStream.time_base units (and guessed if the format cannot
    * provide them). pkt->pts can be AV_NOPTS_VALUE if the video format
    * has B-frames, so it is better to rely on pkt->dts if you do not
    * decompress the payload.
    *
    * @return 0 if OK, < 0 on error or end of file
    */
   int av_read_frame(AVFormatContext *s, AVPacket *pkt);
   ```

4. `av_bsf_send_packet`

   ```c
   /**
    * Submit a packet for filtering.
    *
    * After sending each packet, the filter must be completely drained by calling
    * av_bsf_receive_packet() repeatedly until it returns AVERROR(EAGAIN) or
    * AVERROR_EOF.
    *
    * @param pkt the packet to filter. The bitstream filter will take ownership of
    * the packet and reset the contents of pkt. pkt is not touched if an error occurs.
    * This parameter may be NULL, which signals the end of the stream (i.e. no more
    * packets will be sent). That will cause the filter to output any packets it
    * may have buffered internally.
    *
    * @return 0 on success, a negative AVERROR on error.
    */
   int av_bsf_send_packet(AVBSFContext *ctx, AVPacket *pkt);
   ```

5. `av_bsf_receive_packet`

   ```c
   /**
    * Retrieve a filtered packet.
    *
    * @param[out] pkt this struct will be filled with the contents of the filtered
    *                 packet. It is owned by the caller and must be freed using
    *                 av_packet_unref() when it is no longer needed.
    *                 This parameter should be "clean" (i.e. freshly allocated
    *                 with av_packet_alloc() or unreffed with av_packet_unref())
    *                 when this function is called. If this function returns
    *                 successfully, the contents of pkt will be completely
    *                 overwritten by the returned data. On failure, pkt is not
    *                 touched.
    *
    * @return 0 on success. AVERROR(EAGAIN) if more packets need to be sent to the
    * filter (using av_bsf_send_packet()) to get more output. AVERROR_EOF if there
    * will be no further output from the filter. Another negative AVERROR value if
    * an error occurs.
    *
    * @note one input packet may result in several output packets, so after sending
    * a packet with av_bsf_send_packet(), this function needs to be called
    * repeatedly until it stops returning 0. It is also possible for a filter to
    * output fewer packets than were sent to it, so this function may return
    * AVERROR(EAGAIN) immediately after a successful av_bsf_send_packet() call.
    */
   int av_bsf_receive_packet(AVBSFContext *ctx, AVPacket *pkt);
   ```


## QA

Q: 为什么要用到`AVBitStreamFilter` ?

A: 

1. 关于H264 [H264 编码协议详解]:https://blog.csdn.net/qq_19923217/article/details/83348095

2. h264 的两种封装

   - annexb

     - 一个Access Units(AU)包含一个帧，一帧画面包含一个或多个NALU（Network Abstraction Layer Units）
     - 为了字节对齐，每个NALU有起始码，其中4字节的起始码0x00000001通常标志流的随机访问点SPS， PPS， AUD，IDR，其他nalu使用3字节的起始码
     - 起始码之后的第一个字节为NALU的头部，第7位(禁止位F)一定为0，第5.6位(重要性指示位NRI)标志是否被其他NALU引用，值越大，越重要，第0,1,2,3,4位（NALU类型Type），0:未定义,1-23:NAL单元,24:STAP-A 单一时间组合包,25:STAP-B 单一时间组合包,26:MTAP-16 多个时间组合包,27:MTAP-24 多个时间组合包,28:FU-A 分片,29:FU-B 分片,30-31:未定义,1-12由H.264使用，24-31由其他封包使用（比如rtp），其中0x67位SPS，0x68位PPS，0x65为IDR
     - 其他部分为NALU的payload,长度不定
     - 适用于实时流

   - avcc

     - 视频开始有extradata，包含SPS，PPS
     - 每个NALU前有存储NALU的长度，
     - 适用于视频文件存储


