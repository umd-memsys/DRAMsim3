import tempfile
import ConfigParser

def get_val(config, sec, opt):
    """
    get value from a ini file given the section and option
    the priority here is int, float, boolean and finally string
    """
    try:
        val = config.getint(sec, opt)
    except ValueError:
        try:
            val = config.getfloat(sec, opt)
        except ValueError:
            try:
                val = config.getboolean(sec, opt)
            except ValueError:
                val = config.get(sec, opt)  # shouldn't have any exceptions here..
    return val

def get_val_from_file(config_file, sec, opt):
    """
        a quick way to obtain an option from a config file
    """
    config = ConfigParser.ConfigParser()
    config.read(config_file)
    return get_val(config, sec, opt)

def get_dict(config_file):
    """
    read a ini file specified by config_file and
    return a dict of configs with [section][option] : value structure
    """
    _config_dict = {}
    config = ConfigParser.ConfigParser()
    config.read(config_file)
    for sec in config.sections():
        _config_dict[sec] = {}
        for opt in config.options(sec):
            _config_dict[sec][opt] = get_val(config, sec, opt)
    return _config_dict


def sub_options(config_file, sec, opt, new_value, inplace=False):
    """
        given a config file, replace the specified section, option
        with a new value, and the inplace flag decides whether the 
        config_file will be written or a tempfile handler will be 
        returned, NOTE if inplace is true all the comments in the
        original file will be gone..
    """
    config = ConfigParser.ConfigParser()
    config.read(config_file)
    if not config.has_section(sec):
        config.add_section(sec)
    
    try:
        config.set(sec, opt, str(new_value))
    except ConfigParser.Error:
        print "cannot set sec:%s, option:%s, to %s" % (sec, opt, new_value)
        raise
    if not inplace:
        temp_fp = tempfile.NamedTemporaryFile()
        config.write(temp_fp)
        temp_fp.seek(0)
        return temp_fp 
    else:
        with open(config_file, "wb") as fp:
            config.write(fp)
            config.close()