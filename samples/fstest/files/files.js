
var files = {

  method: function(command, obj)
  {
    if (obj == undefined)
      obj = {};
    obj.method = "files";
    obj.command = command;
    wsconn.sendMessage(obj);
  },

  /*
   * File uploads much simpler than firmware as they're done in one chunk
   * and have no additional validation.
   * 
   * TODO: make sure file goes into currently selected folder
   */
  uploadFiles: function()
  {
    var files = $('#uploadfiles').files;
    if (files.count == 0) {
      window.alert('No files selected to upload');
      return;
    }
  
    for (var i = 0; i < files.length; ++i)
      wsconn.uploadFile(files[i]);
  },
  

  listFiles: function()
  {
    files.method("list");
  },

  fileCaption: function(f)
  {
    dt = new Date(f.mtime * 1000);
    s = f.name;
    while (s.length < 32)
      s += '\u00a0';
    s += ' ' + f.access;
    s += ' ' + f.attr;
    s += ' ' + f.size.toString();
    while (s.length < 48)
      s += '\u00a0';
    s += dt.toString();
    return s;
  },

  addFile: function(f)
  {
    var files = $('#filelist');
    opt = helper.addOption(files, f.name, this.fileCaption(f));
    opt.file = f;
    files.selectedIndex = opt.index
  },

  findFile: function(fn)
  {
    var files= $('#filelist').options;
    for (var i = 0; i < files.length; ++i)
      if (files[i].value == fn)
        return i;
    
    return null;
  },

  onMessage: function(msg)
  {
    if (msg.method != "files")
      return;

    if (msg.command == 'list') {
      if (msg.status == 'success') {
        this.dir = msg.dir;
        msg.files.sort(function(a, b) {
          return a.name.localeCompare(b.name);
        });
        helper.clear($('#filelist'));
        for (var i in msg.files)
          this.addFile(msg.files[i]);
      }
    }
    else if (msg.command == 'info') {
      setStatus(msg);
    }
    else if (msg.command == 'upload') {
      if (msg.status == 'success') {
        var dir = "";
        var sep = msg.name.lastIndexOf('/');
        if (sep >= 0) {
          dir = msg.name.slice(0, sep);
          msg.name = msg.name.slice(sep + 1);
        }
        var i = this.findFile(msg.name);
        if (i >= 0) {
          $('#filelist').options[i].text = this.fileCaption(msg);
          $('#filelist').selectedIndex = i;
        }
        else
          this.addFile(msg);
      }
    }
    else if (msg.command == 'delete') {
      for (var x in msg.files) {
        var f = msg.files[x];
        if (f.status != 'success')
          continue;
        var i = this.findFile(f.name);
        if (i)
          $('#filelist').remove(i);
      }
    }
  },

  
  deleteFiles: function()
  {
    var json = { files: [] };
    if (this.dir)
      json.dir = this.dir;
    var select = $('#filelist');
    for (var i = 0; i < select.options.length; ++i) {
      var opt = select.options[i];
      var file = opt.file;
      if (opt.selected)
        json.files.push({name: file.name});
    }
    
    if (window.confirm("Delete " + json.files.length + " files ?"))
      this.method("delete", json);
  },

  getInfo: function()
  {
    this.method("info");
  },
  
  
  check: function()
  {
    this.method("check");
  },
  
  format: function()
  {
    this.method("format");
  },
  
  editor: function()
  {
    return $("#editor");
  },


  // The current directory
  dir: null,
  // The file object being loaded
  file: {},
  
  filePath: function(name)
  {
    path = name;
    if (this.dir)
      path = this.dir + '/' + path;
    return path;
  },

  loadFile: function()
  {
    this.editor().value = "";
    var x = $("#filelist");
    var sel = x[x.selectedIndex];
    var file = sel.file;
    var path = this.filePath(file.name);
    if (file.attr.includes('D')) {
      this.method("list", {dir: path});
    }
    else {
      file.valid = false;
      this.file = file;
      var self = this;
      helper.load(path, function(data) {
        self.loadContent(data);
      });
    }
  },

  // Callback after file load, data contains content
  loadContent: function(data)
  {
    // this.file set by loadFile() method
    if (!this.file) {
      setStatus("No file loaded");
      return;
    }

    try {
      var ext = fileExt(this.file.name);
      if (ext == 'json') {
        var json = JSON.parse(data);
        this.editor().value = JSON.stringify(json, null, 2);
      }
      else
        this.editor().value = data;
      setStatus(this.file.name + " OK");
      this.file.valid = true;
    }
    catch(err) {
      setStatus(err.message);
    }
  },

  
  /* 
   * If we get websocket file transfers working this is all we need, plus a hook to handle
   * incoming binary messages.
   * 
   * This also gets around the issue of Cross-Domain file loading.
   */
  load_Websocket: function()
  {
    this.editor().value = "";
    var x = $("#filelist");
    var sel = x[x.selectedIndex];
    var file = sel.file;
    var path = thils.filePath(file.name);
    file.valid = false;
    this.file = file

    wsconn.sendMessage( { method:"files", command:"get", path:path });
    setStatus("Loading " + path + " ...");
  },

  checkFile: function()
  {
    this.loadContent(this.editor().value);
  },

  saveFile: function()
  {
    if (!this.file.valid)
      return;

    var path = this.filePath(this.file.name);
    var text = this.editor().value;
    var ext = fileExt(this.file.name);
    if (ext == 'json') {
      var json = JSON.parse(text);
      wsconn.uploadJson(path, json);
    }
    else
      wsconn.uploadFileContent(path, text);
  },

  load: function()
  {
    content.load('files.html', this.listFiles );
  }

};


registerHandler("files", files);
