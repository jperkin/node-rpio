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
          "sources": [
            "src/bcm2835.c"
	  ]
	}]
      ]
    }
  ]
}
