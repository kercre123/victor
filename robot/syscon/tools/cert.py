from Crypto.PublicKey import RSA
import wx
import os

def getPassword():
    app = wx.App()
     
    frame = wx.Frame(None, -1, 'win.py')
    frame.SetSize(200,50)
     
    # Create text input
    dlg = wx.TextEntryDialog(frame, 'Enter password','Certificate Password')
    if dlg.ShowModal() == wx.ID_OK:
        password = dlg.GetValue()
    else:
        password = None
    dlg.Destroy()
    return password

def loadCert(cert_fn = None):
    if not cert_fn:
        if 'COZMO2_CERT' in os.environ:
            cert_fn = os.environ['COZMO2_CERT']
        else:
            cert_fn = os.path.join(os.path.dirname(__file__), "development.pem")

    with open(cert_fn, "r") as cert_fo:
        data = cert_fo.read()
        try:
            return RSA.importKey(data)
        except:
            return RSA.importKey(data, passphrase=getPassword())
