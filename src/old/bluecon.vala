using GLib;

[DBus (name = "org.bluez.Adapter")]
public interface Adapter : GLib.Object {
    public abstract void start_discovery() throws GLib.Error;
    public abstract void stop_discovery() throws GLib.Error;
    public signal void device_found(string param0, GLib.HashTable<string,GLib.Variant> param1);
    public signal void device_disappeared(string param0);
    public signal void property_changed(string param0, GLib.Variant param1);
}

[DBus (name = "org.bluez.Manager")]
public interface Manager : GLib.Object {
    public abstract GLib.ObjectPath default_adapter() throws IOError;
}

[DBus (name="org.bluez.Serial")]
public interface Serial : GLib.Object {
    public abstract string Connect(string param) throws IOError;
    public abstract void Disconnect(string device) throws IOError;
}

class G_ray 
{
    private Manager m;
    private Adapter bluez;
    private GLib.ObjectPath path;
    private Serial ser;
    public string device { get; set; }
    private int ndisc;
    
    private void on_device_found (string address,
                                  HashTable<string, GLib.Variant?> hash)
    {
        string name = (string) hash.lookup("Name");
        if(name == "G-Rays2")
        {
            try  {
                bluez.stop_discovery();
                var dev = "%s/dev_%s".printf(path,address).replace(":","_");
                ser = Bus.get_proxy_sync (BusType.SYSTEM,"org.bluez",dev);
                device = ser.Connect("spp");
            } catch (Error e) {
                stderr.printf ("%s\n", e.message);
            }
            Gtk.main_quit();
        }
    }

    private void on_device_disappeared(string param0) {
        stdout.printf ("Remote device gone (%s)\n", param0);
    }

    private void on_property_changed (string name, GLib.Variant val) {
        if(name == "Discovering")
        {
            if((bool)val == true)
            {
                ndisc++;
                if(ndisc == 3)
                {
                    try  {
                        bluez.stop_discovery();
                    } 
                    catch (Error e) {
                        stderr.printf ("%s\n", e.message);
                    }
                    Gtk.main_quit();
                }
            }
        }
    }
    
    public int connect()  {
        try {
            m = Bus.get_proxy_sync (BusType.SYSTEM, "org.bluez","/");
            path = m.default_adapter();
            bluez = Bus.get_proxy_sync (BusType.SYSTEM, "org.bluez",path);
            bluez.device_found.connect (on_device_found);
            bluez.device_disappeared.connect (on_device_disappeared);
            bluez.property_changed.connect (on_property_changed);
            ndisc = 0;
            bluez.start_discovery();
        } catch (Error e) {
            stderr.printf ("%s\n", e.message);
            return 1;
        }
        return 0;
    }

    public void disconnect () {
        if(device != null)
        {
            try 
            {
                ser.Disconnect(device);
            } catch (Error e) {
                stderr.printf ("%s\n", e.message);
            }
            device=null;
        }
    }
}

