{
  "targets": [
    {
      "target_name": "crobot",
      "sources": [ "src/addon.cc" ],
      "include_dirs": [
        "<!@(node -p \"require('node-addon-api').include\")"
      ],
      "dependencies": [
        "<!(node -p \"require('node-addon-api').gyp\")"
      ],
      "cflags_cc": [ "-std=c++17" ],
      "defines": [ "NAPI_CPP_EXCEPTIONS" ],
      "conditions": [
        ["OS=='mac'", {
          "xcode_settings": {
            "GCC_ENABLE_CPP_EXCEPTIONS": "YES",
            "CLANG_CXX_LANGUAGE_STANDARD": "c++17",
            "MACOSX_DEPLOYMENT_TARGET": "10.13"
          },
          "libraries": [
            "-framework ApplicationServices"
          ]
        }],
        ["OS=='win'", {
          "libraries": [
            "User32.lib"
          ]
        }]
      ]
    }
  ]
}
