{
  "targets": [
    {
      "target_name": "rpio",
      "include_dirs": [ "<!(node -e \"require('nan')\")" ],
      "sources": [
        "src/rpio.cc"
      ],
      "conditions": [
        ["OS == 'linux'", {
          "cflags": ["-fpermissive",  "-fexceptions", "-Wmisleading-indentation"],
          "cflags_cc": ["-fpermissive",  "-fexceptions", "-Wmisleading-indentation"],
          "sources": [
            "src/bcm2835.cpp",
            "src/sunxi.cpp"
	  ]
	}]
      ]
    }
  ]
}
