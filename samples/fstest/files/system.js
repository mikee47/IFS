
var system = {

  method: function(command)
  {
    enableElements(content.ctx, false);
    wsconn.sendMessage({ method:"system", command:command });
  },
  
  click: function(btn)
  {
    this.method(btn.id);
  },

  reload: function(devices)
  {
    this.method("reload", devices ? { devices: true } : null);
  },

  dbgConsole: function()
  {
    dbgConsole = $('#dbgConsole').checked;
  },

  dbgMessage: function()
  {
    dbgMessage= $('#dbgMessage').checked;
    if (!dbgMessage)
      helper.clear($('#message'));
  },

  load: function()
  {
    content.load("system.html", function() {
      $('#dbgConsole').checked = dbgConsole;
      $('#dbgMessage').checked = dbgMessage;
    });
  },
  
  onMessage: function(msg)
  {
    if (msg.method != 'system')
      return;

    enableElements(content.ctx, true);
//      enableElement($('#' + msg.command), true);
    
    if (msg.command == 'info')
      setStatus(msg);
  }

};

registerHandler("system", system);