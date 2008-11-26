
using System;
using System.Runtime.InteropServices;

namespace TAPCfg {
	public class EthernetDevice : IDisposable {
		private const int MTU = 1522;

		private IntPtr handle;
		private bool disposed = false;

		public EthernetDevice() {
			handle = tapcfg_init();
		}

		public void Start() {
			Start(null);
		}

		public void Start(string deviceName) {
			tapcfg_start(handle, deviceName);
		}

		public EthernetFrame Read() {
			byte[] buffer = new byte[MTU];

			int ret = tapcfg_read(handle, buffer, buffer.Length);
			if (ret < 0) {
				/* Handle error in reading */
			} else if (ret == 0) {
				/* Handle EOF in reading */
			}

			return new EthernetFrame(buffer);
		}

		public void Write(EthernetFrame frame) {
			byte[] buffer = frame.Data;

			int ret = tapcfg_write(handle, buffer, buffer.Length);
			if (ret < 0) {
				/* Handle error in writing */
			} else if (ret != buffer.Length) {
				/* Handle not full write */
			}
		}

		public bool Enabled {
			get {
				int ret = tapcfg_iface_get_status(handle);
				if (ret != 0) {
					return true;
				} else {
					return false;
				}
			}
			set {
				if (value) {
					tapcfg_iface_change_status(handle, 1);
				} else {
					tapcfg_iface_change_status(handle, 0);
				}
			}
		}

		public string DeviceName {
			get { return tapcfg_get_ifname(handle); }
		}

		public void Dispose() {
			Dispose(true);
			GC.SuppressFinalize(this);
		}

		protected virtual void Dispose(bool disposing) {
			if (!disposed) {
				if (disposing) {
					// Managed resources can be disposed here
				}

				tapcfg_stop(handle);
				tapcfg_destroy(handle);
				handle = IntPtr.Zero;

				disposed = true;
			}
		}

		private static void Main(string[] args) {
			EthernetDevice dev = new EthernetDevice();
			dev.Start();
			dev.Enabled = true;

			System.Threading.Thread.Sleep(100000);
		}

		[DllImport("libtapcfg")]
		private static extern IntPtr tapcfg_init();
		[DllImport("libtapcfg")]
		private static extern void tapcfg_destroy(IntPtr tapcfg);

		[DllImport("libtapcfg")]
		private static extern int tapcfg_start(IntPtr tapcfg, string ifname);
		[DllImport("libtapcfg")]
		private static extern void tapcfg_stop(IntPtr tapcfg);

/*
		[DllImport("libtapcfg")]
		private static extern int tapcfg_can_read(IntPtr tapcfg);
		[DllImport("libtapcfg")]
		private static extern int tapcfg_can_write(IntPtr tapcfg);
*/

		[DllImport("libtapcfg")]
		private static extern int tapcfg_read(IntPtr tapcfg, byte[] buf, int count);
		[DllImport("libtapcfg")]
		private static extern int tapcfg_write(IntPtr tapcfg, byte[] buf, int count);

		[DllImport("libtapcfg")]
		private static extern string tapcfg_get_ifname(IntPtr tapcfg);

		[DllImport("libtapcfg")]
		private static extern int tapcfg_iface_get_status(IntPtr tapcfg);
		[DllImport("libtapcfg")]
		private static extern int tapcfg_iface_change_status(IntPtr tapcfg, int enabled);
		[DllImport("libtapcfg")]
		private static extern int tapcfg_iface_set_ipv4(IntPtr tapcfg, string addr, byte netbits);
		[DllImport("libtapcfg")]
		private static extern int tapcfg_iface_set_ipv6(IntPtr tapcfg, string addr, byte netbits);
	}
}
