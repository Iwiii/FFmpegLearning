#include <stdio.h>
#include <libavutil/log.h>
#include <libavformat/avformat.h>
#include <libavfilter/avfilter.h>



int main(int argc, char *argv[])
{
    int ret;//返回值
    char *src_filename = NULL;//源文件名称
    char *dst_filename = NULL;//目标文件名称
    int  video_stream_index = -1;
    size_t write_length = 0;
    FILE *dst_fd = NULL;//目标文件
    AVPacket pkt;
    AVFormatContext          *fmt_ctx = NULL;
    AVBSFContext             *bsf_ctx = NULL;
    const AVBitStreamFilter    *filter = NULL;
    
    
    
    av_log_set_level(AV_LOG_INFO);
    
    // # 判断传入的参数是否合法
    // 1. 判断数目是否合法
    if(argc < 3){
        av_log(NULL, AV_LOG_ERROR, "The count of parameters should be more than three!\n");
        return -1;
    }
    //2.判断参数是否合法
    src_filename = argv[1];
    dst_filename = argv[2];
   if(src_filename == NULL || dst_filename == NULL){
       av_log(NULL, AV_LOG_ERROR, "src or dts file is null, plz check them!\n");
       return -1;
   }
 
    // # 打开文件获取上下文 dump meta信息
    // 3. 打开src文件
   // av_register_all();//新版已经遗弃,兼容
    /*open input media file, and allocate format context*/
    if((ret = avformat_open_input(&fmt_ctx, src_filename, NULL, NULL)) < 0){
        av_log(NULL, AV_LOG_ERROR, "Could not open source file: %s,%s\n",
               src_filename,
               av_err2str(ret));
        return -1;
    }
    // 2. dump infomation of src
    av_dump_format(fmt_ctx, 0, src_filename, 0);
    
    // 4. 找到流
    video_stream_index = av_find_best_stream(fmt_ctx, AVMEDIA_TYPE_VIDEO, -1, -1, NULL, 0);
    if(video_stream_index < 0){
        av_log(NULL, AV_LOG_DEBUG, "Could not find %s stream in input file %s\n",
               av_get_media_type_string(AVMEDIA_TYPE_VIDEO),
               src_filename);
        avformat_close_input(&fmt_ctx);
        return -1;
    }
    
    // 5. 配置过滤器
    // i 找到相应解码器的过滤器 此处直接h264_mp4toannexb
    filter = av_bsf_get_by_name("h264_mp4toannexb");
    if (!filter) {
        av_log(NULL, AV_LOG_ERROR, "Unknow bitstream filter");
    }
    // ii 给过滤器分配内存
    av_bsf_alloc(filter, &bsf_ctx);
    // iii 添加解码器属性
    AVCodecParameters  *codecpar = NULL;
    if (fmt_ctx->streams[video_stream_index]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
        codecpar = fmt_ctx->streams[video_stream_index]->codecpar;
    }
    avcodec_parameters_copy(bsf_ctx->par_in,codecpar);
    // iv 初始化过滤器上下文
    av_bsf_init(bsf_ctx);
   
    // 5. open dstfile 循环读取包 过滤 写入文件
    dst_fd = fopen(dst_filename, "wb");
    if (!dst_fd) {
        av_log(NULL, AV_LOG_DEBUG, "Could not open destination file %s\n", dst_filename);
        goto __fail;
        return -1;
    }
    while(av_read_frame(fmt_ctx, &pkt) >=0 ){
        if(pkt.stream_index == video_stream_index){
            ret = av_bsf_send_packet(bsf_ctx, &pkt);
            if (ret<0) {
                av_log(NULL, AV_LOG_WARNING, "something is wrong:%s",av_err2str(ret));
            }
            while (av_bsf_receive_packet(bsf_ctx, &pkt)==0) {
                write_length = fwrite(pkt.data, 1, pkt.size, dst_fd);
                if (write_length!=pkt.size) {
                    av_log(NULL, AV_LOG_WARNING, "the wrote data size is not correct!");
                }
            }
        }
        //减引用计数 释放pkt
        av_packet_unref(&pkt);
    }
    
__fail:
    {
        avformat_close_input(&fmt_ctx);
        if(dst_fd) {
            fclose(dst_fd);
        }
        av_bsf_free(&bsf_ctx);
    }
    
    return 0;
}
