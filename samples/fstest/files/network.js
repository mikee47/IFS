
var network = {
    
  info: {},

  method: function(command, obj)
  {
    if (obj == undefined)
      obj = {};
    obj.method = "network";
    obj.command = command;
    wsconn.sendMessage(obj);
  },
  
  scan: function(refresh)
  {
    showElement($('#nwset'), false);
    var select = $('#networks');
    if (refresh || select.length == 0) {
      helper.clear(select);
      helper.addOption(select, "", "Scanning...");
      network.method("scan");
    }
  },

  selchange: function()
  {
    var ssid = $('#networks').value;
    showElement($('#nwset'), ssid != "");
  },
  
  showPassword: function()
  {
    $('#appwd').type = $('#showPassword').checked ? 'text' : 'password';
  },
  
  
  configure: function()
  {
    var ssid = $('#networks').value;
    if (ssid)
      this.method("config", { "ssid":ssid, "password":$('#appwd').value });
  },
  
  
  onMessage: function(msg)
  {
    if (msg.method != 'network')
      return;
    
    if (msg.command == 'info') {
      network.info = msg;
    }
    else if (msg.command == 'scan') {
      if (msg.status == 'success') {
        var networks = $('#networks');
        helper.clear(networks);
        helper.addOption(networks, "", "Select Network:");
        for (var i in msg.networks) {
          var nw = msg.networks[i];
          var s = nw.ssid + ' (' + nw.auth + ')';
          if (nw.ssid == this.info.ssid)
            s = "* " + s;
          helper.addOption(networks, nw.ssid, s);
        }
      }
    }
  },
  
  initialise: function()
  {
    this.method("info");
    this.scan();
  },
  
  load: function()
  {
    var self = this;
    content.load("network.html", function() {
      self.initialise();
    });
  }
  
};

registerHandler("network", network);

