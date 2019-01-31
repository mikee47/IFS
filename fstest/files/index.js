function sidenav()
{
  return $("#sidenav");
}

var content = {
  ctx: null,

  clear: function()
  {
    helper.clear(this.ctx);
  },

  append: function(s)
  {
    this.ctx.innerHTML += s;
  },
  
  load: function(filename, callback)
  {
    helper.load(filename, function(data) {
      content.ctx.innerHTML = data;
      if (callback)
        callback();
    });
  }
};


function initialise()
{
  content.ctx = $('#content');
  content.clear();
  updateState();
  showElements($('#pages'), false);
}


function updateState()
{
//  showElement($('#main'), config.loaded);
  var e = (wsconn.state() == WS_CLOSED);
  enableElements($('#main'), e);
  showElement($('#connect'), e);
  showElement($('#password'), e);
  showElement($('#username'), e || $('#username').value);
  e = (wsconn.state() == WS_CONNECTED);
  showElement($('#disconnect'), e);
  enableElement($('#disconnect'), e);
  e = e && ($('#pages').childElementCount > 1);
  $('#menu').style.display = e ? "inline-block" : "none";
}

function ws_stateChanged(state)
{
  if (state == WS_CONNECTED) {
    wsconn.login($('#username').value, $('#password').value);
  }
  else if (state != WS_CONNECTING) {
    fillMenu([]);
    content.clear();
  }
  updateState();
}


var handlers = {
  files:      { name: "Files" },
  network:    { name: "Network" },
  system:     { name: "System" }
};

// Handler calls this to register itself after loading
function registerHandler(method, obj)
{
  helper.log("registerHandler " + method);
  var handler = handlers[method];
  var msg = "Handler for method '" + method + "' ";
  if (handler) {
    handler.obj = obj;
    setStatus(msg + "loaded");
    activeHandler = method;
    obj.load();
  }
  else
    setStatus(msg + "not found");
}


var activeHandler = "Files";

function ws_login(msg)
{
//      password.value = null;
  fillMenu(msg.methods);
  updateState();
  if (msg.methods.indexOf(activeHandler) < 0)
    activeHandler = msg.methods[0];
  select(activeHandler);
}

function ws_message(msg)
{
  if (activeHandler) {
    var handler = handlers[activeHandler];
    if (handler.obj)
      handler.obj.onMessage(msg);
  }
}


function fillMenu(methods)
{
  var pages = $('#pages');
  helper.clear(pages);
  for (var i = 0; i < methods.length; ++i) {
    var m = methods[i];
    var handler = handlers[m];
    if (!handler)
      continue;
    var a = document.createElement("A");
    a.href = "javascript:void(0)";
    a.id = m;
    a.onclick = function() {
      select(this.id) 
    };
    a.innerHTML = handler.name;
    pages.appendChild(a);
  }
}


/*
 * @param opt tag of handler or element whose ID is tag of handler
 */
function select(method)
{
  activeHandler = null;

  closeNav();
  content.clear();

  var handler = handlers[method];
  if (!handler) {
    content.append("No handler for " + method);
    return;
  }

  if (handler.obj) {
    activeHandler = method;
    handler.obj.load();
  }
  else
	  helper.loadScript(method);
}


function openNav()
{
  sidenav().style.width = "100%"; //"200px";
  document.body.style.backgroundColor = "rgba(0,0,0,0.4)";
}

function closeNav()
{
  sidenav().style.width = "0";
  document.body.style.backgroundColor = "white";
}
