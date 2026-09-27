// config.d

import core.stdc.stdio : stdout_fileno = stdout;
import core.time;
import std.algorithm;
import std.array;
import std.conv;
import std.datetime.date;
import std.datetime.systime : Clock, SysTime;
import std.datetime.timezone : LocalTime;
import std.exception;
import std.file;
import std.path;
import std.process : environment;
import std.stdio;
import std.string;

enum NetworkMode : string
{
  NEITHER = "NONE",
  UDP     = "UDP",
  SSH     = "SSH",
}

struct Config
{
  NetworkMode mode;
  string host;
  uint port;
}

class ConfigManagementException : Exception
{
  this(string msg, string file = __FILE__, size_t line = __LINE__)
  {
    super(msg, file, line);
  }
}

class ConfigManager
{
  /* Internal configuration file management */
  private Config config;
  private string config_path;
  private Config default_config;
  private File f;


  private void deserialize()
  {
    /* reads config file into configuration data */

    // INI parser: step-by-step:
    //
    // 1. Parse into array of lines
    // 2. Snip all lines at "#"
    // 3. Drop all empty lines
    // 4. Seek until "[network]"
    // 5. Read by line until "[" (the next category in the config)
    // 6. Match `line.split("=")[0] to required fields
    // 7. Copy `line.split("=")[1] to respective field (cast if needed)
    // 8. If a cast fails, or the necesary fields weren't satisfied, throw


    //MonoTime t_start, t_stop;
    //Duration elapsed;
    //t_start = MonoTime.currTime;

    this.config.host = "";
    this.config.port = 0;
    this.config.mode = NetworkMode.NEITHER;

    this.f = File(this.config_path, "r");

    string[] lines;

    foreach(line; this.f.byLine)
      lines ~= line.idup;

    this.f.close();

    //t_stop = MonoTime.currTime;
    //elapsed = t_stop - t_start;

    //writefln("deserialize (I/O): %s", elapsed);

    //t_start = MonoTime.currTime;

    foreach(line; lines)
      line = line.split("#")[0];

    for(int i = 0; i < lines.length; i++)
      if(!lines[i].length)
        lines = remove(lines, i++);

    import std.algorithm.searching : find;
    long start_idx, stop_idx;
    for(int i = 0; i < lines.length; i++)
    {
      if(find(lines[i], "[network]"))
      {
        start_idx = ++i;

        for(int j = cast(int)lines.length - 1; j > i; j--)
          if(find(lines[j], "["))
          {
            stop_idx = j;
            break;
          }
        break;
      }
    }
    for(long i = start_idx; i < stop_idx; i++)
    {
      auto s = findSplit(lines[i], "=");

      if(s[0] == "host")
        this.config.host = s[2].replace(" ", "");

      else if(s[0] == "port")
        this.config.port = to!int(s[2].replace(" ", ""));

      else if(s[0] == "mode")
        this.config.mode = to!NetworkMode(s[2].replace(" ", ""));

      // ADDITIONAL CONFIGS ARE ADDED HERE
    }

    if(this.config.host == "" || this.config.mode == NetworkMode.NEITHER
    ||(this.config.mode == NetworkMode.UDP && !this.config.host))
      throw new ConfigManagementException("config requirements weren't satisfied");

    //t_stop  = MonoTime.currTime;
    //elapsed = t_stop - t_start;

    //writefln("deserialize:\t\t%s", elapsed);
  }

  private void serialize(Config c)
  {
    /* writes current configuration into config file */

    // INI writer: step-by-step
    //
    // 1. generate lines from values
    // 2. clean file
    // 3. write "[network]\n"
    // 4. write generated lines

    //MonoTime t_start, t_stop;
    //Duration elapsed;

    //t_start = MonoTime.currTime;

    string[] lines;

    lines ~= "[network]";
    lines ~= format("mode=%s", c.mode);
    lines ~= format("host=%s", c.host);

    if(c.mode == NetworkMode.UDP)
      lines ~= format("port=%s", c.port);
    // ADDITIONAL CONFIGS ARE ADDED HERE

    //t_stop = MonoTime.currTime;
    //elapsed = t_stop - t_start;

    //writefln("serialize: %s", elapsed);
    
    //t_start = MonoTime.currTime;

    this.f = File(this.config_path, "w");

    foreach(line; lines)
      this.f.writeln(line);

    this.f.close();

    //t_stop = MonoTime.currTime;
    //elapsed = t_stop - t_start;

    //writefln("serialize (I/O):\t\t\t\t%s", elapsed);
  }

  Config getconf()
  {
    return this.config;
  }

  void setconf(Config c)
  {
    this.config = c;
    serialize(this.config);
  }

  string getconf_path()
  {
    return config_path;
  }

  Config getconf_default()
  {
    return this.default_config;
  }

  debug void setconf_path(string s)
  {
    /* sets path to config file */
    config_path = s;
  }

  this(string config_path=format("%s/.config/limeade/limeade.ini", environment["HOME"]))
  {
    this.default_config.mode = NetworkMode.SSH;
    this.default_config.host = "127.0.0.1";
    this.default_config.port = 12_046;

    this.config_path = config_path;

    string dir = dirName(this.config_path);
    if(!dir.exists)
      mkdir(dir);

    try
    {
      deserialize();
    }
    catch (Exception e)
    {
      if(this.f.isOpen())
        this.f.close();
      this.config = this.default_config;
      serialize(this.config);
    }
  }

  unittest
  {
    auto c = new ConfigManager();
    c.setconf(c.getconf());
    c.destroy();
  }
}

class Logger
{
  /* Internal log management */
  FILE *stream;
  private char* blue, green, cyan, red, yellow, reset;

  void set_colors(bool s)
  {
    if(s)
    {
      this.blue   = cast(char*)"\033[34m";
      this.cyan   = cast(char*)"\033[36m";
      this.green  = cast(char*)"\033[32m";
      this.red    = cast(char*)"\033[31m";
      this.yellow = cast(char*)"\033[33m";
      this.reset  = cast(char*)"\033[39m";
    } else
      blue = cyan = green = red = yellow = reset = cast(char*)"";
  }

  this(FILE* stream=stdout_fileno, bool colors=true)
  {
    this.stream = stream;

    if(this.stream == stdout_fileno && colors)
    {
      this.blue   = cast(char*)"\033[34m";
      this.cyan   = cast(char*)"\033[36m";
      this.green  = cast(char*)"\033[32m";
      this.red    = cast(char*)"\033[31m";
      this.yellow = cast(char*)"\033[33m";
      this.reset  = cast(char*)"\033[39m";
    } else
      blue = cyan = green = red = yellow = reset = cast(char*)"";
  }

  private string get_time()
  {
    return Clock.currTime(LocalTime()).toISOExtString;
  }

  void dbg(string msg)
  {
    fprintf(this.stream, "[%s%s%s][%sDEBUG%s]: %s\n", this.blue,
            this.get_time().toStringz, this.reset, this.cyan, this.reset,
            msg.toStringz);
  }

  void info(string msg)
  {
    fprintf(this.stream, "[%s%s%s][%sINFO%s]:  %s\n", this.blue,
            this.get_time().toStringz, this.reset, this.green, this.reset,
            msg.toStringz);
  }

  void warn(string msg)
  {
    fprintf(this.stream, "[%s%s%s][%sWARN%s]:  %s\n", this.blue,
            this.get_time().toStringz, this.reset, this.yellow, this.reset,
            msg.toStringz);
  }


  void err(string msg)
  {
    fprintf(this.stream, "[%s%s%s][%sERROR%s]: %s\n", this.blue,
            this.get_time().toStringz, this.reset, this.red, this.reset,
            msg.toStringz);
  }

  unittest
  {
    auto l = new Logger();
    l.dbg("This is a debugging message.");
    l.info("This is an informational message.");
    l.warn("This is a warning message.");
    l.err("This is an error message.");
    l.set_colors(false);
    l.dbg("This is a debugging message.");
    l.info("This is an informational message.");
    l.warn("This is a warning message.");
    l.err("This is an error message.");
  }
}

void main()
{

}
