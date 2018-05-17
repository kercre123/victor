#!/usr/bin/env python
from __future__ import print_function
"""
Example implementation of Anki Victor Update engine.
"""
__author__ = "Daniel Casner <daniel@anki.com>"

import sys
import os
import urllib2
import subprocess
import tarfile
import zlib
import shutil
import ConfigParser
from Crypto.Cipher import AES
from Crypto.Util.number import bytes_to_long, long_to_bytes
from hashlib import sha256, md5
from collections import OrderedDict

BOOT_DEVICE = "/dev/block/bootdevice/by-name"
STATUS_DIR = "/run/update-engine"
MOUNT_POINT = "/mnt/sdcard"
ANKI_REV_FILE = "/anki/etc/revision"
ANKI_VER_FILE = "/anki/etc/version"
EXPECTED_DOWNLOAD_SIZE_FILE = os.path.join(STATUS_DIR, "expected-download-size")
EXPECTED_WRITE_SIZE_FILE = os.path.join(STATUS_DIR, "expected-size")
PROGRESS_FILE = os.path.join(STATUS_DIR, "progress")
ERROR_FILE = os.path.join(STATUS_DIR, "error")
DONE_FILE = os.path.join(STATUS_DIR, "done")
MANIFEST_FILE = os.path.join(STATUS_DIR, "manifest.ini")
MANIFEST_SIG = os.path.join(STATUS_DIR, "manifest.sha256")
BOOT_STAGING = os.path.join(STATUS_DIR, "boot.img")
DELTA_STAGING = os.path.join(STATUS_DIR, "delta.bin")
OTA_PUB_KEY = "/anki/etc/ota.pub"
OTA_ENC_PASSWORD = "/anki/etc/ota.pas"
DD_BLOCK_SIZE = 1024*256
SUPPORTED_MANIFEST_VERSIONS = ["0.9.2", "0.9.3", "0.9.4", "0.9.5"]

DEBUG = False
SIGNATURE_VALIDATION_DISABLED = False


def safe_delete(name):
    if os.path.isfile(name):
        os.remove(name)
    elif os.path.isdir(name):
        shutil.rmtree(name)

def clear_status():
    "Clear everything out of the status directory"
    if os.path.isdir(STATUS_DIR):
        for filename in os.listdir(STATUS_DIR):
            os.remove(os.path.join(STATUS_DIR, filename))


def write_status(file_name, status):
    "Simple function to (over)write a file with a status"
    with open(file_name, "w") as output:
        output.write(str(status))
        output.truncate()


def die(code, text):
    "Write out an error string and exit with given status code"
    write_status(ERROR_FILE, text)
    if DEBUG:
        sys.stderr.write(str(text))
        sys.stderr.write(os.linesep)
    exit(code)


def open_slot(partition, slot, mode):
    "Opens a partition slot"
    if slot == "f":
        assert mode == "r"  # No writing to F slot
        if partition == "system":
            label = "recoveryfs"
        elif partition == "boot":
            label = "recovery"
        else:
            raise ValueError("Unknown partition")
    else:
        label = partition + "_" + slot
    return open(os.path.join(BOOT_DEVICE, label), mode + "b")


def zero_slot(target_slot):
    "Writes zeros to the first block of the destination slot boot and system to ensure they aren't booted"
    assert target_slot == 'a' or target_slot == 'b'  # Make sure we don't zero f
    zeroblock = b"\x00"*DD_BLOCK_SIZE
    open_slot("boot", target_slot, "w").write(zeroblock)
    open_slot("system", target_slot, "w").write(zeroblock)


def call(*args):
    "Simple wrapper arround subprocess.call to make ret=0 -> True"
    return subprocess.call(*args) == 0


def verify_signature(file_path_name, sig_path_name, public_key):
    "Verify the signature of a file based on a signature file and a public key with openssl"
    openssl = subprocess.Popen(["/usr/bin/openssl",
                                "dgst",
                                "-sha256",
                                "-verify",
                                public_key,
                                "-signature",
                                sig_path_name,
                                file_path_name],
                               shell=False,
                               stdout=subprocess.PIPE,
                               stderr=subprocess.PIPE)
    ret_code = openssl.wait()
    openssl_out, openssl_err = openssl.communicate()
    return ret_code == 0, ret_code, openssl_out, openssl_err


def get_prop(property_name):
    "Gets a value from the property server via subprocess"
    getprop = subprocess.Popen(["/usr/bin/getprop", property_name], shell=False, stdout=subprocess.PIPE)
    if getprop.wait() == 0:
        return getprop.communicate()[0].strip()
    return None


def get_cmdline():
    "Returns /proc/cmdline arguments as a dict"
    cmdline = open("/proc/cmdline", "r").read()
    ret = {}
    for arg in cmdline.split(' '):
        try:
            key, val = arg.split('=')
        except ValueError:
            ret[arg] = None
        else:
            if val.isdigit():
                val = int(val)
            ret[key] = val
    return ret


def get_slot(kernel_command_line):
    "Get the current and target slots from the kernel commanlines"
    suffix = kernel_command_line.get("androidboot.slot_suffix", '_f')
    if suffix == '_a':
        return 'a', 'b'
    elif suffix == '_b':
        return 'b', 'a'
    else:
        return 'f', 'a'


def get_manifest(fileobj):
    "Returns config parsed from INI file in filelike object"
    config = ConfigParser.ConfigParser()
    config.readfp(fileobj)
    return config


def get_section_encryption(manifest, section):
    "Returns the encryption setting of the section"
    try:
        enc = manifest.getint(section, "encryption")
    except ConfigParser.NoOptionError:
        enc = 0
    return enc


def get_password():
    "Returns the encryption password"
    return open(OTA_ENC_PASSWORD, "rb").read()



def open_url_stream(url):
    "Open a URL as a filelike stream"
    try:
        assert url.startswith("http")  # Accepts http and https but not ftp or file
        os_version = get_prop("ro.anki.version")
        victor_version = get_prop("ro.anki.victor.version")
        if '?' in url:  # Already has a querry string
            if not url.endswith('?'):
                url += '&'
        else:
            url += '?'
        url += "emresn={0:s}&ankiversion={1:s}&victorversion={2:s}".format(
                get_prop("ro.serialno"),
                os_version,
                victor_version)
        request = urllib2.Request(url)
        opener = urllib2.build_opener()
        opener.addheaders = opener.addheaders = [('User-Agent', 'Victor/{0:s}'.format(os_version))]
        return opener.open(request)
    except Exception as e:
        die(203, "Failed to open URL: " + str(e))


def make_tar_stream(fileobj, open_mode="r|"):
    "Converts a file like object into a streaming tar object"
    try:
        return tarfile.open(mode=open_mode, fileobj=fileobj)
    except Exception as e:
        die(204, "Couldn't open contents as tar file " + str(e))


class ShaFile(object):
    "A fileobject wrapper that calculates a sha256 digest as it's processed"

    def __init__(self, fileobj):
        "Setup the wrapper"
        self.fileobj = fileobj
        self.sum = sha256()
        self.len = 0

    def read(self, *args):
        "Read function wrapper which adds calculates the shasum as it goes"
        block = self.fileobj.read(*args)
        self.sum.update(block)
        self.len += len(block)
        return block

    def digest(self):
        "Calculate the hexdigest"
        return self.sum.hexdigest()

    def bytes(self):
        "Return the total bytes read from the file"
        return self.len


class AESCtr(object):
    "AES Counter Object"
    def __init__(self, nonce):
        self.c = bytes_to_long(nonce)

    def __call__(self):
        r = long_to_bytes(self.c)
        self.c += 1
        return r


class CryptStream(ShaFile):
    "A file like object for decrypting openssl created AES-256-CTR data"

    def __init__(self, fileobj, password, calc_sha=False):
        "Initalize the CryptStream from a source file object and password"
        ShaFile.__init__(self, fileobj)
        def evp_bytes_to_key(password, salt, key_len, iv_len):
            "Private key parsing function"
            m = md5()
            m.update(password)
            m.update(salt)
            dtot = m.digest()
            d = [dtot]
            while len(dtot) < (iv_len + key_len):
                m = md5()
                m.update(d[-1])
                m.update(password)
                m.update(salt)
                d.append(m.digest())
                dtot = dtot + d[-1]
            return bytes(dtot[:key_len]), bytes(dtot[key_len:key_len+iv_len])

        magic = fileobj.read(8)
        if magic != 'Salted__':
            raise ValueError('Openssl magic not found')
        salt = fileobj.read(8)
        (key, iv) = evp_bytes_to_key(password, salt, 32, 16)
        self.aes_decrypter = AES.new(key, AES.MODE_CTR, counter=AESCtr(iv))
        self.do_sha = calc_sha

    def read(self, *args):
        "Read method for crypt stream"
        block = self.aes_decrypter.decrypt(self.fileobj.read(*args))
        if self.do_sha:
            self.sum.update(block)
        self.len += len(block)
        return block


def GZStreamGenerator(fileobj, block_size, wbits):
    "A generator for decompressing gzip data from a filelike object"
    decompressor = zlib.decompressobj(wbits)
    block = fileobj.read(block_size)
    while len(block) >= block_size:
        yield decompressor.decompress(block, block_size)
        if len(decompressor.unconsumed_tail) < block_size:
            block = decompressor.unconsumed_tail + fileobj.read(block_size - len(decompressor.unconsumed_tail))
    output_block = decompressor.decompress(block, block_size)
    while len(output_block) == block_size:
        yield output_block
        output_block = decompressor.decompress(decompressor.unconsumed_tail, block_size)
    yield output_block


def handle_boot_system(target_slot, manifest, tar_stream):
    "Process 0.9.2 format manifest files"
    total_size = manifest.getint("BOOT", "bytes") + manifest.getint("SYSTEM", "bytes")
    write_status(EXPECTED_WRITE_SIZE_FILE, total_size)
    written_size = 0
    write_status(PROGRESS_FILE, written_size)
    # Extract boot image
    if DEBUG:
        print("Boot")
    boot_ti = tar_stream.next()
    if not boot_ti.name.endswith("boot.img.gz"):
        die(200, "Expected boot.img.gz to be next in tar but found \"{}\"".format(boot_ti.name))
    wbits = manifest.getint("BOOT", "wbits")
    digest = sha256()
    with open(BOOT_STAGING, "wb") as boot_fh:
        src_file = tar_stream.extractfile(boot_ti)
        enc = get_section_encryption(manifest, "BOOT")
        if enc == 1:
            src_file = CryptStream(src_file, get_password())
        elif enc != 0:
            die(210, "Unsupported enc value")
        for boot_block in GZStreamGenerator(src_file, DD_BLOCK_SIZE, wbits):
            boot_fh.write(boot_block)
            digest.update(boot_block)
            written_size += len(boot_block)
            write_status(PROGRESS_FILE, written_size)
            if DEBUG:
                sys.stdout.write("{0:0.3f}\r".format(float(written_size)/float(total_size)))
                sys.stdout.flush()
    # Verify boot hash
    if digest.hexdigest() != manifest.get("BOOT", "sha256"):
        zero_slot(target_slot)
        die(209, "Boot image hash doesn't match signed manifest")
    # Extract system images
    if DEBUG:
        print("System")
    system_ti = tar_stream.next()
    if not system_ti.name.endswith("sysfs.img.gz"):
        die(200, "Expected sysfs.img.gz to be next in tar but found \"{}\"".format(system_ti.name))
    wbits = manifest.getint("SYSTEM", "wbits")
    digest = sha256()
    with open_slot("system", target_slot, "w") as system_slot:
        src_file = tar_stream.extractfile(system_ti)
        enc = get_section_encryption(manifest, "SYSTEM")
        if enc == 1:
            src_file = CryptStream(src_file, get_password())
        elif enc != 0:
            die(210, "Unsupported enc value")
        for system_block in GZStreamGenerator(src_file, DD_BLOCK_SIZE, wbits):
            system_slot.write(system_block)
            digest.update(system_block)
            written_size += len(system_block)
            write_status(PROGRESS_FILE, written_size)
            if DEBUG:
                sys.stdout.write("{0:0.3f}\r".format(float(written_size)/float(total_size)))
                sys.stdout.flush()
    # Verify system image hash
    if digest.hexdigest() != manifest.get("SYSTEM", "sha256"):
        zero_slot(target_slot)
        die(209, "System image hash doesn't match signed manifest")
    # Actually write the boot image now
    with open(BOOT_STAGING, "rb") as src:
        with open_slot("boot", target_slot, "w") as dst:
            dst.write(src.read())


def copy_slot(partition, src_slot, dst_slot):
    "Copy the contents of a partition slot from one to the other"
    with open_slot(partition, src_slot, "r") as src:
        with open_slot(partition, dst_slot, "w") as dst:
            buffer = src.read(DD_BLOCK_SIZE)
            while len(buffer) == DD_BLOCK_SIZE:
                dst.write(buffer)
                buffer = src.read(DD_BLOCK_SIZE)
            dst.write(buffer)


def update_build_props(mount_point):
    "Updates (or creates) a property in the build.prop file specified"
    # Get the Anki Victor info
    victor_build_ver = open(os.path.join(mount_point, ANKI_VER_FILE), "r").read().strip()
    victor_build_rev = open(os.path.join(mount_point, ANKI_REV_FILE), "r").read().strip()
    build_prop_path_name = os.path.join(mount_point, "build.prop")
    # Get all the old OS properties
    props = OrderedDict()
    for key, value in [p.strip().split('=') for p in open(build_prop_path_name, "r").readlines()]:
        props[key] = value
    os_version = props['ro.anki.version']
    os_build_timestamp = props['ro.build.version.release']
    os_rev = ""  # TODO find this somewhere
    props["ro.revision"] = "anki-{VICTOR_BUILD_REV}_os-{REV}".format(VICTOR_BUILD_REV=victor_build_rev, REV=os_rev)
    props["ro.anki.victor.version"] = victor_build_ver
    version_id = "v{VICTOR_BUILD_VERSION}_os{OS_VERSION}".format(
        VICTOR_BUILD_VERSION=victor_build_ver,
        OS_VERSION=os_version
    )
    build_id = "v{VICTOR_BUILD_VERSION}{VICTOR_REV_TAG}_os{OS_VERSION}{REV_TAG}-{BUILD_TIMESTAMP}".format(
        VICTOR_BUILD_VERSION=victor_build_ver,
        VICTOR_REV_TAG="-" + victor_build_rev if victor_build_rev else "",
        OS_VERSION=os_version,
        REV_TAG="-" + os_rev if os_rev else "",
        BUILD_TIMESTAMP=os_build_timestamp
    )
    props["ro.build.fingerprint"] = build_id
    props["ro.build.id"] = build_id
    props["ro.build.display.id"] = version_id

    with open(build_prop_path_name, "w") as propfile:
        propfile.write("\n".join(["=".join(prop) for prop in props.items()]))
        propfile.write("\n")


def handle_delta(current_slot, target_slot, manifest, tar_stream):
    "Apply a delta update to the boot and system partitions"
    current_version = get_prop("ro.anki.version")
    delta_base_version = manifest.get("DELTA", "base_version")
    if current_version != delta_base_version:
        die(211, "My version is {} not {}".format(current_version, delta_base_version))
    total_size = manifest.getint("DELTA", "bytes")
    write_status(EXPECTED_WRITE_SIZE_FILE, total_size)
    written_size = 0
    write_status(PROGRESS_FILE, 0)
    # Extract delta file
    if DEBUG:
        print("Delta")
    delta_ti = tar_stream.next()
    if not delta_ti.name.endswith("delta.bin.gz"):
        die(200,
            "Expected delta.bin.gz to be next in tar but found \"{}\""
            .format(delta_ti.name))
    wbits = manifest.getint("DELTA", "wbits")
    digest = sha256()
    with open(DELTA_STAGING, "wb") as delta_fh:
        src_file = tar_stream.extractfile(delta_ti)
        enc = get_section_encryption(manifest, "DELTA")
        if enc == 1:
            src_file = CryptStream(src_file, get_password())
        elif enc != 0:
            die(210, "Unsupported enc value")
        for delta_block in GZStreamGenerator(src_file, DD_BLOCK_SIZE, wbits):
            delta_fh.write(delta_block)
            digest.update(delta_block)
            written_size += len(delta_block)
            write_status(PROGRESS_FILE, written_size)
            if DEBUG:
                sys.stdout.write("{0:0.3f}\r".format(float(written_size)/float(total_size)))
                sys.stdout.flush()
    # Verify delta hash
    if digest.hexdigest() != manifest.get("DELTA", "sha256"):
        safe_delete(DELTA_STAGING)
        die(209, "delta.bin hash doesn't match manifest value")
    paycheck = subprocess.Popen(["/usr/bin/paycheck.py",
                                 "--do-not-truncate-to-expected-size",
                                 DELTA_STAGING,
                                 os.path.join(BOOT_DEVICE, "boot_" + target_slot),
                                 os.path.join(BOOT_DEVICE, "system_" + target_slot),
                                 os.path.join(BOOT_DEVICE, "boot_" + current_slot),
                                 os.path.join(BOOT_DEVICE, "system_" + current_slot)],
                                shell=False,
                                stdout=subprocess.PIPE,
                                stderr=subprocess.PIPE)
    ret_code = paycheck.wait()
    paycheck_out, paycheck_err = paycheck.communicate()
    safe_delete(DELTA_STAGING)
    if ret_code != 0:
        die(219,
            "Failed to apply delta. paycheck.py returned {0}\n{1}\n{2}"
            .format(ret_code, paycheck_out, paycheck_err))



def handle_anki(current_slot, target_slot, manifest, tar_stream):
    "Update the Anki folder only"
    write_status(EXPECTED_WRITE_SIZE_FILE, 4)  # We're faking progress here with just stages 0-N
    write_status(PROGRESS_FILE, 0)
    if DEBUG:
        print("Copying system from {} to {}".format(current_slot, target_slot))
    copy_slot("system", current_slot, target_slot)
    write_status(PROGRESS_FILE, 1)
    if DEBUG:
        print("Installing new Anki")
    if not call(["mount", os.path.join(BOOT_DEVICE, "system" + "_" + target_slot), MOUNT_POINT]):
        die(208, "Couldn't mount target system partition")
    try:
        anki_path = os.path.join(MOUNT_POINT, "anki")
        shutil.rmtree(anki_path)
        write_status(PROGRESS_FILE, 2)
        anki_ti = tar_stream.next()
        src_file = tar_stream.extractfile(anki_ti)
        enc = get_section_encryption(manifest, "ANKI")
        if enc == 1:
            sha_fh = CryptStream(src_file, get_password(), True)
        elif enc != 0:
            die(210, "Unsupported enc value")
        else:
            sha_fh = ShaFile(src_file)
        anki_tar = make_tar_stream(sha_fh, "r|" + manifest.get("ANKI", "compression"))
        anki_tar.extractall(MOUNT_POINT)
        update_build_props(MOUNT_POINT)
    finally:
        call(["umount", MOUNT_POINT])
    write_status(PROGRESS_FILE, 3)
    # Verify anki tar hash
    if sha_fh.bytes() != manifest.getint("ANKI", "bytes"):
        zero_slot(target_slot)
        die(209, "Anki archive wrong size")
    if sha_fh.digest() != manifest.get("ANKI", "sha256"):
        zero_slot(target_slot)
        die(209, "Anki archive didn't match signed manifest")
    # Copy over the boot partition since we passed
    if DEBUG:
        print("Sig passed, installing kernel")
    copy_slot("boot", current_slot, target_slot)
    write_status(PROGRESS_FILE, 4)


def update_from_url(url):
    "Updates the inactive slot from the given URL"
    # Figure out slots
    cmdline = get_cmdline()
    current_slot, target_slot = get_slot(cmdline)
    if DEBUG:
        print("Target slot is", target_slot)
    # Make status directory
    if not os.path.isdir(STATUS_DIR):
        os.mkdir(STATUS_DIR)
    # Open URL as a tar stream
    stream = open_url_stream(url)
    content_length = stream.info().getheaders("Content-Length")[0]
    write_status(EXPECTED_DOWNLOAD_SIZE_FILE, content_length)
    with make_tar_stream(stream) as tar_stream:
        # Get the manifest
        if DEBUG:
            print("Manifest")
        manifest_ti = tar_stream.next()
        if not manifest_ti.name.endswith('manifest.ini'):
            die(200, "Expected manifest.ini at beginning of download, found \"{0.name}\"".format(manifest_ti))
        with open(MANIFEST_FILE, "wb") as manifest:
            manifest.write(tar_stream.extractfile(manifest_ti).read())
        manifest_sig_ti = tar_stream.next()
        if not manifest_sig_ti.name.endswith('manifest.sha256'):
            die(200, "Expected manifest signature after manifest.ini. Found \"{0.name}\"".format(manifest_sig_ti))
        with open(MANIFEST_SIG, "wb") as signature:
            signature.write(tar_stream.extractfile(manifest_sig_ti).read())
        verification_status = verify_signature(MANIFEST_FILE, MANIFEST_SIG, OTA_PUB_KEY)
        if not verification_status[0]:
            if not SIGNATURE_VALIDATION_DISABLED:
                die(209,
                    "Manifest failed signature validation, openssl returned {1:d} {2:s} {3:s}".format(*verification_status))
            elif DEBUG:
                sys.stderr.write("ERROR 209 (suppressed): Manifest failed signature validation.{linesep} {1:d} {2:s} {3:s}{linesep}".format(*verification_status, linesep=os.linesep))
        # Manifest was signed correctly
        manifest = get_manifest(open(MANIFEST_FILE, "r"))
        # Inspect the manifest
        if manifest.get("META", "manifest_version") not in SUPPORTED_MANIFEST_VERSIONS:
            die(201, "Unexpected manifest version")
        if DEBUG:
            print("Updating to version {}".format(manifest.get("META", "update_version")))
        # Mark target unbootable
        if not call(['/bin/bootctl', current_slot, 'set_unbootable', target_slot]):
            die(202, "Could not mark target slot unbootable")
        zero_slot(target_slot)  # Make it doubly unbootable just in case
        num_images = manifest.getint("META", "num_images")
        if num_images == 2:
            if manifest.has_section("BOOT") and manifest.has_section("SYSTEM"):
                handle_boot_system(target_slot, manifest, tar_stream)
            else:
                die(201, "Two images specified but couldn't find boot or system")
        elif num_images == 1:
            if manifest.has_section("DELTA"):
                handle_delta(current_slot, target_slot, manifest, tar_stream)
            elif manifest.has_section("ANKI"):
                handle_anki(current_slot, target_slot, manifest, tar_stream)
            else:
                die(201, "One image specified but not DELTA or ANKI")
        else:
            die(201, "Unexpected manifest configuration")
    stream.close()
    # Ensure new images are synced to disk
    if not call(["/bin/sync"]):
        die(208, "Couldn't sync OS images to disk")
    # Mark the slot bootable now
    if not call(["/bin/bootctl", current_slot, "set_active", target_slot]):
        die(202, "Could not set target slot as active")
    write_status(DONE_FILE, 1)


if __name__ == '__main__':
    if len(sys.argv) == 1:  # Clear the output directory
        clear_status()
        exit(0)
    elif len(sys.argv) == 3 and sys.argv[2] == '-v':
        DEBUG = True

    if DEBUG:
        update_from_url(sys.argv[1])
    else:
        try:
            update_from_url(sys.argv[1])
        except zlib.error as decompressor_error:
            die(205, "Decompression error: " + str(decompressor_error))
        except IOError as io_error:
            die(208, "IO Error: " + str(io_error))
        except Exception as e:
            die(219, e)
        exit(0)
