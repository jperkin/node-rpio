{
  "targets": [
    {
      "target_name": "rpio",
      "include_dirs": [ "<!(node -e \"require('nan')\")" ],
      "sources": [
        "src/bcm2835.c",
        "src/sunxi.c",
        "src/rpio.cc"
      ]
    }
  ]
}
