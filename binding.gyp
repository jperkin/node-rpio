{
  "targets": [
    {
      "target_name": "rpio",
      "include_dirs": [ "<!(node -e \"require('nan')\")" ],
      "sources": [
        "src/bcm2835.c",
        "src/rpio.cc"
      ]
    },
    {
      "target_name": "npio",
      "include_dirs": [ "<!(node -e \"require('nan')\")" ],
      "sources": [
        "src/npio.cc",
        "src/wiringPi.h",
        "src/wiringPi.c"
      ]
    }
  ]
}
