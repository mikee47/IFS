/*
 * STS Websocket IO library
 */
const PING_INTERVAL = 30000;
const MSG_ERROR = 'Error';
const MSG_DISCONNECTED = 'Disconnected';
const MSG_CONNECTING = 'Connecting...';
const MSG_CONNECTED = 'Connected.';
const UPLOAD_CHUNKSIZE = 16384;

const MSG_PING = '?';
const MSG_PONG = '#';

// readyState
const WS_CONNECTING = 0
const WS_CONNECTED  = 1
const WS_CLOSING    = 2
const WS_CLOSED     = 3

// Local storage tag
const CID_TAG = "sts-demo.cid";

var dbgMessage = false;
var dbgConsole = true;

// Some very basic jQuery-esque stuff - at some point will probably just use jQuery
var helper = {
  query: function(id, ctx)
  {
    if (!ctx)
      ctx = document;

    if (id[0] == '#')
      return ctx.getElementById(id.substring(1));
    else if (id[0] == '.')
      return ctx.getElementsByClassName(id.substring(1));
    else
      return ctx.getElementsByTagName(id);
  },
  
  clear: function(el)
  {
    el.innerHTML = "";
  },
  
  log: function(s)
  {
    if (dbgConsole)
      console.log(s);
  },

  find: function(arr, key, value)
  {
    for (var i in arr)
      if (arr[i][key] == value)
        return arr[i];
      
    return null;
  },
  
  addOption: function(select, value, text)
  {
    var opt = document.createElement('OPTION');
    opt.value = value;
    opt.text = text;
    select.add(opt);
    return opt;
  },

  load: function(filename, callback)
  {
    var xmlhttp = new XMLHttpRequest();
    xmlhttp.onerror = function() {
      setStatus("Load '" + filename + "': " + this.statusText);
    };
    xmlhttp.onload = function() {
      setStatus("Load '" + filename + "': " + this.statusText);
      if (this.status == 200)
        if (callback)
          callback(this.response);
    };

    xmlhttp.open("GET", filename + "?cid=" + wsconn.cid, true);
    xmlhttp.responseType = 'text';
    xmlhttp.send(); 
    setStatus("Loading " + filename + " ...");
  },


  loadScript: function(name, callback)
  {
    var filename = name + ".js";
    if ($("#" + filename)) {
      if (callback)
        callback();
      return;
    }

    var js = document.createElement("SCRIPT");
    js.id = filename;
    js.type = "text/javascript";
    js.src = filename + "?cid=" + wsconn.cid;
    js.onerror = function() {
      $("#scripts").removeChild(js);
      setStatus("Load ERROR"); 
    };
    js.onload = function() {
      if (callback)
        callback();
    }
    $("#scripts").appendChild(js);
    setStatus("Loading " + filename  + " ...");
  }
};


const $ = helper.query;


function setStatus(msg)
{
  helper.log(msg);

  if (typeof msg === "object")
    $('#status').innerHTML = jsonFormat(msg);
  else
    $('#status').innerText = msg;
}

function setMessage(msg)
{
  $('#message').innerHTML = msg;
}


// Websocket Connection object
var wsconn = {

  // The websocket
  ws: null,
  
  // Ping to check activity at intervals
  pingTimeout: 0,

  state: function()
  {
    return this.ws ? this.ws.readyState : WS_CLOSED;
  },

    
  queuePing: function()
  {
    clearTimeout(this.pingTimeout);
    var self = this;
    this.pingTimeout = setTimeout(function() {
      if (self.checkState()) {
        self.ws.send(MSG_PING);
        self.queuePing();
      }
    }, PING_INTERVAL);
  },

  
  connect: function()
  {
    if (this.ws) {
      if (this.ws.readyState != WS_CLOSED)
        return;
  
      helper.log("set ws = null");
      ws = null;
    }

    var server = location.hostname;
    var path = location.pathname;
    if (location.port)
      server += ":" + location.port;
    var protocol = (location.protocol == "https:") ? "wss:" : "ws:";
    var n = path.lastIndexOf("/");
    if (n < 0)
      path = "/";
    else if (n > 0)
      path = path.substring(0, n + 1);
    var url = protocol + "//" + server + path + "ws";

    helper.log("Connecting to " + url);
    try {
      this.ws = new WebSocket(url);
    }
    catch(err) {
      setStatus(err.message);
      return;
    }
  
    setStatus('Connecting...');
    ws_stateChanged(WS_CONNECTING);
    
    var self = this;

    this.ws.onopen = function () {
      helper.log('WS Open');
      setStatus(MSG_CONNECTED);
      ws_stateChanged(WS_CONNECTED);
    };

    this.ws.onerror = function () {
      helper.log('WS Error');
      setStatus(MSG_ERROR);
    };
  
    this.ws.onmessage = function (e) {
      helper.log("< " + e.data);

      if (e.data == MSG_PONG)
        return;
  
      var msg = JSON.parse(e.data);

      if (dbgMessage)
        setMessage(jsonFormat(msg));
  
      setStatus(msg.method + "." + msg.command + ": " + (msg.error ? msg.error.text : msg.status));

      if (msg.method == "auth") {
        if (msg.command == "login") {
          if (msg.status == "success") {
            self.cid = msg.cid.toString(16);
            window.localStorage.setItem(CID_TAG, msg.cid);
            ws_login(msg);
            return;
          }
          self.ws.close();
          self.ws = null;
        }
      }
      else
        ws_message(msg);
    };
  
    this.ws.onclose = function(){
      clearTimeout(self.pingTimeout);
      helper.log('WS Closed');
//      setStatus(MSG_DISCONNECTED);
  
      ws_stateChanged(WS_CLOSED);
      self.ws = null;
      return "closed";
    };
    
  },


  login: function(username, password)
  {
    var req = { method:"auth", command:"login", name:username, password:password };
    req.cid = window.localStorage.getItem(CID_TAG);
    this.sendMessage(req);
  },


  disconnect: function()
  {
    if (this.ws) {
      helper.log("disconnect()");
      setStatus("");
      this.ws.close();
      this.ws = null;
    }
  },
  
  
  checkState: function()
  {
    if (this.ws && this.ws.readyState == WS_CONNECTED)
      return true;
  
    this.disconnect();
    return false;
  },
  
  
  sendMessage: function(json)
  {
    if (!this.checkState())
      return;
    
    var s = JSON.stringify(json);
    helper.log("> " + s);
    this.ws.send(s);
    this.queuePing();
  },
  
  sendBinary: function(blob)
  {
    this.ws.send(blob);
  },
  

  uploadFileContent: function(filename, content)
  {
    var blob = new Blob([content]);
    this.sendMessage({ method:"files", command:"upload", name:filename, size:blob.size });
    this.sendBinary(blob);
  },

  uploadFile: function(file)
  {
    this.sendMessage({ method:"files", command:"upload", name:file.name, size:file.size });
    this.sendBinary(file);
  },

  uploadJson: function(filename, json)
  {
    this.uploadFileContent(filename, JSON.stringify(json));
  }


};



function setButton(btn, clsName)
{
  btn.className = clsName;
  btn.disabled = (clsName == 'disabled');
}

function setButtons(div, clsName, state)
{
  var buttons = $("*", div);
  for (var i = 0; i < buttons.length; ++i)
    setButton(buttons[i], clsName, state);
}

function enableElement(el, state)
{
  if (state)
    el.classList.remove("disabled");
  else
    el.classList.add("disabled");
    el.disabled = !state;
}

function enableElements(div, state)
{
  if (!div)
    return;
  var buttons = $("*", div);
  for (var i = 0; i < buttons.length; ++i)
    enableElement(buttons[i], state);
}


function showElement(btn, visible)
{
  btn.style.display = visible ? "inline-block" : "none";
}


function showElements(div, state)
{
  var buttons = $("*", div);
  for (var i = 0; i < buttons.length; ++i)
    showElement(buttons[i], state);
}


function fileExt(filename)
{
  return filename.split('.').pop();
}


function jsonFormat(json)
{
  if (Array.isArray(json))
    if (typeof json[0] != 'object')
      return '[' + json.join(", ") + "]";

  var s = '<ul>';
  for (var key in json) {
    s += '<li>' + key;
    var value = json[key];
    if (typeof value === 'object')
      s += ': ' + jsonFormat(value);
    else
      s += ' = ' + value + '</li>';
  }
  return s + '</ul>';
}


