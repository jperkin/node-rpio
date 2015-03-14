{
  "variables": {
    "node_addon": '<!(node -p -e "require(\'path\').dirname(require.resolve(\'addon-layer\'))")'
  },
  "targets": [
    {
      "target_name": "shim",
      "dependencies": [ '<(node_addon)/binding.gyp:addon-layer' ],
      "include_dirs": [ '<(node_addon)/include' ],
      "sources": [
        "src/bcm2835.c",
        "src/rpio.c"
      ]
    }
  ]
}
