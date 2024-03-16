#!/usr/bin/env python3
"""
Download example modules for TrackMeister.

This downloads the top-40 tracks (by user ratings) from modarchive.org,
plus some hand-picked example modules for specific tests (like modules
with very many channels, or with outstanding pattern data visualizations).
"""
import argparse
import io
import re
import shutil
import sys
import os
import urllib.request
import zipfile

MODARCHIVE_TOP_LIST_URL = "https://modarchive.org/index.php?request=view_chart&query=topscore"

EXTRA_DOWNLOADS_RAW = [
    "https://api.modarchive.org/downloads.php?moduleid=198842#bacter_vs_saga_musix_-_wonderland.it"
]
EXTRA_DOWNLOADS_ZIP = [
    ("https://files.scene.org/get/parties/2018/revision18/tracked-music/logicoma-wrecklamation.zip",
     "Logicoma - Wrecklamation.xm"),
]

class DownloadHelper:
    def __init__(self, verbose=True):
        self.verbose = verbose
        try:
            self.existing_files = { self.filekey(fn): fn for fn in os.listdir(".") if not fn.startswith(".") }
        except EnvironmentError:
            self.existing_files = {}

    @staticmethod
    def filekey(fn):
        if re.match(r'\d\d(_|-|\s)', fn): fn = fn[3:]
        return fn.lower()

    @staticmethod
    def makename(fn, number=None):
        fn = os.path.basename(fn)
        if not number: return fn
        return f"{number:02d}-{fn}"

    def handle_existing(self, fn, number=None):
        if not fn: return False
        fn = os.path.basename(fn)
        oldname = self.existing_files.get(fn.lower())
        newname = self.makename(fn, number)
        if oldname == newname:
            if self.verbose: print(f"{newname} - already downloaded")
            return True
        if oldname:
            if self.verbose: print(f"{newname} <= {oldname}")
            try:
                os.rename(oldname, newname)
            except EnvironmentError as e:
                print(f"ERROR: failed to rename '{oldname}' to '{newname}' - {e}", file=sys.stderr)
            return True
        return False

    def download(self, url, number=None):
        # try to guess the filename from the URL; if we already downloaded it,
        # we can then avoid hitting the server first
        fn = None
        if ("modarchive.org" in url) and ('#' in url):
            # modarchive.org download URLs usually contain the filename as a fragment
            fn = url.rpartition('#')[-1]
        else:
            # else, strip off the query and extract the last filename component
            fn = url.partition('?')[0].rpartition('/')[-1]
        # exclude a few common "definitely wrong" file extensions
        if os.path.splitext(fn)[-1].lstrip('.').lower() in ("php", "html", "htm"):
            fn = None
        # use the guess to decide on skipping or renaming the file
        if self.handle_existing(fn, number):
            return

        # download the file
        try:
            if self.verbose: print(url, end='\r')
            with urllib.request.urlopen(url) as f:
                # determine the target filename
                cd = f.headers.get('Content-Disposition', "")
                fn_pos = cd.find('filename=')
                if fn_pos >= 0:
                    fn = cd[fn_pos:].rpartition('=')[-1].partition(';')[0].strip()
                newname = self.makename(fn, number)
                if self.verbose: print(f"{newname} <= {url}")
                # download the file contents
                with open(newname, 'wb') as out:
                    shutil.copyfileobj(f, out)
        except EnvironmentError as e:
            print(f"ERROR: module download failed - {e}", file=sys.stderr)

    def download_zip(self, url, fn, number=None):
        if self.handle_existing(fn, number):
            return
        dest = self.makename(fn)
        if self.verbose: print(f"{dest} <= {url}")

        # download
        try:
            with urllib.request.urlopen(url) as f:
                data = f.read()
        except EnvironmentError as e:
            print(f"ERROR: archive download failed - {e}", file=sys.stderr)

        # extract
        try:
            with zipfile.ZipFile(io.BytesIO(data), 'r') as z:
                data = z.read(fn)
        except EnvironmentError as e:
            print(f"ERROR: archive extraction failed - {e}", file=sys.stderr)

        # store
        try:
            with open(dest, 'wb') as f:
                f.write(data)
        except EnvironmentError as e:
            print(f"ERROR: file output failed - {e}", file=sys.stderr)


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__,
                                     formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("outdir", nargs='?',
                        default=os.path.join(os.path.dirname(sys.argv[0]), "examples"),
                        help="output directory (default: repository's examples/ subdirectory)")
    parser.add_argument("-q", "--quiet", action='store_true',
                        help="don't print status messages")
    args = parser.parse_args()

    # prepare (and change into) output directory
    outdir = os.path.normpath(os.path.abspath(args.outdir))
    if not os.path.isdir(outdir):
        try:
            if not args.quiet: print(f"creating output directory {outdir}")
            os.makedirs(outdir)
        except EnvironmentError as e:
            print(f"FATAL: failed to create destination directory - {e}", file=sys.stderr)
            sys.exit(1)
    else:
        if not args.quiet: print(f"output directory {outdir} already exists")
    try:
        os.chdir(outdir)
    except EnvironmentError as e:
        print(f"FATAL: failed to change into destination directory - {e}", file=sys.stderr)
        sys.exit(1)
    dl = DownloadHelper(not(args.quiet))

    # query the top-40 from modarchive.org
    try:
        if not args.quiet: print("parsing", MODARCHIVE_TOP_LIST_URL)
        with urllib.request.urlopen(MODARCHIVE_TOP_LIST_URL) as f:
            doc = f.read().decode('utf-8', 'replace')
        # extract URLs
        urls = []
        for url in re.findall(r'<a \s+ [^>]* href=" ([^"]+) "', doc, flags=re.I+re.X):
            if ("download" in url.lower()) and not(url in urls):
                urls.append(url)
        if not urls:
            print(f"WARNING: no download links found in modarchive.org charts", file=sys.stderr)
    except EnvironmentError as e:
        print(f"ERROR: failed to download modarchive.org charts - {e}", file=sys.stderr)
        urls = []  # non-fatal error -> don't panic, pretend that there are no links

    # download all the modules
    for rank, url in enumerate(urls, 1):
        dl.download(url, rank)
    for url in EXTRA_DOWNLOADS_RAW:
        dl.download(url)
    for url, fn in EXTRA_DOWNLOADS_ZIP:
        dl.download_zip(url, fn)
