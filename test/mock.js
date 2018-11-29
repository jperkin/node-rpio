/*
 * Copyright (c) 2018 Jonathan Perkin <jonathan@perkin.org.uk>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

var tap = require('tap');
var rpio = require('../lib/rpio.js');

tap.pass('basic tap sanity check');

tap.test('initialise mock mode', function (t) {
	rpio.init({mock: 'raspi-3'});
	t.end();
});

tap.test('set up custom warning handler', function (t) {
	rpio.on('warn', function() {});
	t.end();
});

tap.test('rpio open', function (t) {
	rpio.open(11, rpio.INPUT, rpio.PULL_DOWN);
	rpio.open(12, rpio.OUTPUT, rpio.HIGH);
	rpio.open(13, rpio.OUTPUT);
	t.end();
});

tap.test('rpio polling', function (t) {
	rpio.poll(15, function () {});
	rpio.poll(15, null);
	t.end();
});
