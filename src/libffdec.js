/*! 
Copyright(c) 2020, LGPLV3
https://github.com/lishaoliang/ffdec.js
https://gitee.com/lishaoliang/ffdec.js
*/

var ffdecjs = (function () {

var _hello = function(n/*number*/){console.log('ffdecjs hello:[', n, ']');return n;};
var _set_hidden = function(hidden){console.log('ffdecjs_set_hidden', hidden);};
var _quit = function(status/*number*/){console.log('ffdecjs quit');return status;};
var _control = function(cmd/*string*/, lparam/*string*/, wparam/*string*/){return ''};
var _open = function(name/*string*/, param/*string*/){return 0;};
var _close = function(){return 0;};
var _get_head = function(){return null;};
var _pop_head = function(){return 0;};
var _write_test_file = function(path_name/*string*/){return 0;};
var _print_file = function(path_name/*string*/){return 0;};


var _preRun = function(){
    // the first
    //console.log('ffdecjs preRun')

};

var _onRuntimeInitialized = function(){
    // before main			

    _hello = Module.cwrap('ffdecjs_hello', 'number', ['number']);
    _set_hidden = Module.cwrap('ffdecjs_set_hidden', 'number', ['number']);
    _quit = Module.cwrap('ffdecjs_quit', 'number', ['number']);
    _control = Module.cwrap('ffdecjs_control', 'string', ['string','string','string']);
    _open = Module.cwrap('ffdecjs_open', 'number', ['string', 'string']);
    _close = Module.cwrap('ffdecjs_close', 'number', []);
    _get_head = Module.cwrap('ffdecjs_get_head', 'number', []);
    _pop_head = Module.cwrap('ffdecjs_pop_head', 'number', []);
    _write_test_file = Module.cwrap('ffdecjs_write_test_file', 'number', ['string']);
    _print_file = Module.cwrap('ffdecjs_print_file', 'number', ['string']);

    ffdecjs.hello(200);
};

var _postRun = function(){
    // after main

    // Test
    ffdecjs.hello(300);
};

var _print = (function() {
    return function(text) {
        if (arguments.length > 1) text = Array.prototype.slice.call(arguments).join(' ');
        // These replacements are necessary if you render to raw HTML
        //text = text.replace(/&/g, "&amp;");
        //text = text.replace(/</g, "&lt;");
        //text = text.replace(/>/g, "&gt;");
        //text = text.replace('\n', '<br>', 'g');
        console.log(text);
    };
  })();

var _printErr = function(text){
    if (arguments.length > 1) text = Array.prototype.slice.call(arguments).join(' ');
    console.log(text);
};


var _setStatus = function(text){
    //if (arguments.length > 1) text = Array.prototype.slice.call(arguments).join(' ');
    //console.log(text);
};


// 监听页面是否激活
var listenerVisibility = function() {
    // https://www.cnblogs.com/csuwujing/p/10315309.html
    var hiddenProperty = 'hidden' in document ? 'hidden' :    
                        'webkitHidden' in document ? 'webkitHidden' :    
                        'mozHidden' in document ? 'mozHidden' :    
                        null;

    var visibilityChangeEvent = hiddenProperty.replace(/hidden/i, 'visibilitychange');

    document.addEventListener(visibilityChangeEvent, function(){
        if (!document[hiddenProperty]) {    
            //console.log('document show');
            _set_hidden(false);
        }else{
            //console.log('document hidden');
            _set_hidden(true);
        }
    });
};

listenerVisibility();

// _get_head函数给出的媒体数据类型
var FFDECJS_MEDIA_YUV420P  = 1;
var FFDECJS_MEDIA_ARGB    = 2;
var FFDECJS_MEDIA_RGBA    = 3;
var FFDECJS_MEDIA_JPEG    = 101;

var get_head_frame = function(){
    // _get_head 函数返回的是C结构体
    // 从C结构中, 将数据转换成JS对象
    // 原始C数据结构定义见: ./src_c/main_app/main_app.h : ffdecjs_media_t
    var ptr = _get_head();
    if(ptr){
        var w = Module.HEAP32[ptr + 0 >> 2];               // 宽(int32_t): 偏移.0, 大小4
        var h = Module.HEAP32[ptr + 4 >> 2];               // 高(int32_t): 偏移.4, 大小4
        var time_low = Module.HEAP32[ptr + 8 >> 2];        // 帧时间(int64_t): 偏移.8, 大小8
        var time_high = Module.HEAP32[ptr + 12 >> 2];

        var dtype = Module.HEAP32[ptr + 16 >> 2];          // data(图片)类型(int32_t): 偏移.16, 大小4
        var p_data = Module.HEAP32[ptr + 20 >> 2];         // data内存指针(uint8_t*): 偏移.20, 大小4
        var data_len = Module.HEAP32[ptr + 24 >> 2];       // data长度(int32_t): 偏移.24, 大小4

        var otpye = Module.HEAP32[ptr + 28 >> 2];          // 解码之后的图像类型(int32_t): 偏移.28, 大小4
        
        // 
        var frame = {
            w : w,
            h : h,
            time : new Uint32Array([time_low, time_high]), // 0x8000000000000000 为无效值
            dtype : '',
            otpye : ''
        }
        
        switch(otpye){
        case FFDECJS_MEDIA_YUV420P:
            {
                var p_y = Module.HEAP32[ptr + 32 >> 2];        // Y内存指针(uint8_t*): 偏移.32, 大小4
                var p_u = Module.HEAP32[ptr + 36 >> 2];        // U内存指针(uint8_t*): 偏移.36, 大小4
                var p_v = Module.HEAP32[ptr + 40 >> 2];        // V内存指针(uint8_t*): 偏移.40, 大小4

                var pitch_y = Module.HEAP32[ptr + 44 >> 2];    // Y行跨距(int32_t): 偏移.44, 大小4
                var pitch_u = Module.HEAP32[ptr + 48 >> 2];    // U行跨距(int32_t): 偏移.48, 大小4
                var pitch_v = Module.HEAP32[ptr + 52 >> 2];    // V行跨距(int32_t): 偏移.52, 大小4

                frame.otpye = 'YUV420P';

                frame.y = Module.HEAPU8.subarray(p_y, p_y + pitch_y * h);
                frame.u = Module.HEAPU8.subarray(p_u, p_u + pitch_u * h / 2);
                frame.v = Module.HEAPU8.subarray(p_v, p_v + pitch_v * h / 2);

                frame.pitch_y = pitch_y;
                frame.pitch_u = pitch_u;
                frame.pitch_v = pitch_v;
            }
            break;
        }

        return frame
    }

    return null
};

return {
    quit : _quit,
    preRun : _preRun,
    onRuntimeInitialized : _onRuntimeInitialized,
    postRun : _postRun,
    print : _print,
    printErr : _printErr,
    setStatus : _setStatus,

    hello : function(n/*number*/){ return _hello(n); },

//    control : function(cmd/*string*/,lparam/*string*/,wparam/*string*/){
//        var ptr = _control(cmd, lparam, wparam);
//
//        if (ptr) {
//            var s = new String(ptr);
//            _free(ptr);
//            return s;
//        };
//
//        return '';
//    },

    open : function(name/*string*/, param/*string*/){
        _close(); // 先关闭
        return _open(name, param);
    },

    close : function(){
        return _close();
    },

    get_next_frame : function(){
        var frame = get_head_frame();
        if (frame) {
            // 提取到数据之后, 将队列首帧释放掉
            _pop_head();
        }

        return frame;
    },

/*
    get_frame : function(cb){
        g_get_frame_callback = cb;

        var ptr = _get_frame();

        console.log("v1", UTF8ToString(ffdecjs.HEAP32[ptr >> 2]));
        console.log("v2", UTF8ToString(ffdecjs.HEAP32[ptr + 4 >> 2]));
        console.log("w", ffdecjs.HEAP32[ptr + 8 >> 2]);
        console.log("h", ffdecjs.HEAP32[ptr + 12 >> 2]);

        var d = new Uint8Array(ffdecjs.HEAPU8.buffer, ffdecjs.HEAP32[ptr >> 2], 7);

        console.log("d:", d);
        return 0;
    },
*/
    write_test_file : function(path_name/*string*/) {
        return _write_test_file(path_name);
    },

    print_file : function(path_name/*string*/){
        return _print_file(path_name);
    }
};

})();

var Module = typeof Module !== 'undefined' ? Module : ffdecjs;
