"use strict";

let binding = require("./build/Release/binding");
binding._validate();
module.exports = binding;
