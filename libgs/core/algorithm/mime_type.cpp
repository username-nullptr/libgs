
/************************************************************************************
*                                                                                   *
*   Copyright (c) 2024 Xiaoqiang <username_nullptr@163.com>                         *
*                                                                                   *
*   This file is part of LIBGS                                                      *
*   License: MIT License                                                            *
*                                                                                   *
*   Permission is hereby granted, free of charge, to any person obtaining a copy    *
*   of this software and associated documentation files (the "Software"), to deal   *
*   in the Software without restriction, including without limitation the rights    *
*   to use, copy, modify, merge, publish, distribute, sublicense, and/or sell       *
*   copies of the Software, and to permit persons to whom the Software is           *
*   furnished to do so, subject to the following conditions:                        *
*                                                                                   *
*   The above copyright notice and this permission notice shall be included in      *
*   all copies or substantial portions of the Software.                             *
*                                                                                   *
*   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR      *
*   IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,        *
*   FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE     *
*   AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER          *
*   LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,   *
*   OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE   *
*   SOFTWARE.                                                                       *
*                                                                                   *
*************************************************************************************/

#include "mime_type.h"
#include "libgs/core/app_utls.h"
#include "base.h"

#include <unordered_map>
#include <fstream>

namespace libgs
{

#undef TEXT

#define APPLICATION  "application/"
#define AUDIO        "audio/"
#define CHEMICAL     "chemical/"
#define IMAGE        "image/"
#define INODE        "inode/"
#define MESSAGE      "message/"
#define MODEL        "model/"
#define MULTIPART    "multipart/"
#define TEXT         "text/"
#define VIDEO        "video/"
#define INTERFACE    "interface"

static suffix_type_map g_suffix_hash
{
	{ ".1"             , TEXT "troff"                                                                  },
	{ ".3ds"           , IMAGE "x-3ds"                                                                 },
	{ ".3fr"           , IMAGE "x-raw-hasselblad"                                                      },
	{ ".3g2"           , VIDEO "3gpp2"                                                                 },
	{ ".3gp"           , VIDEO "3gpp"                                                                  },
	{ ".3mf"           , APPLICATION "vnd.ms-package.3dmanufacturing-3dmodel+xml"                      },
	{ ".4th"           , TEXT "x-forth"                                                                },
	{ ".669"           , AUDIO "x-mod"                                                                 },
	{ ".6pl"           , TEXT "x-perl"                                                                 },
	{ ".7z"            , APPLICATION "x-7z-compressed"                                                 },
	{ ".8svx"          , AUDIO "8svx"                                                                  },
	{ ".ML"            , TEXT "x-ocaml"                                                                },
	{ ".MYD"           , APPLICATION "x-mysql-misam-data"                                              },
	{ ".MYI"           , APPLICATION "x-mysql-misam-compressed-index"                                  },
	{ ".R"             , TEXT "x-rsrc"                                                                 },
	{ ".TIF"           , IMAGE "tiff"                                                                  },
	{ ".XXX"           , APPLICATION "octet-stream"                                                    },
	{ ".Z"             , APPLICATION "x-compress"                                                      },
	{ ".aa"            , AUDIO "x-pn-audibleaudio"                                                     },
	{ ".ac3"           , AUDIO "ac3"                                                                   },
	{ ".acsm"          , APPLICATION "vnd.adobe.adept+xml"                                             },
	{ ".ada"           , TEXT "x-ada"                                                                  },
	{ ".adf"           , MULTIPART "appledouble"                                                       },
	{ ".afa"           , APPLICATION "x-astrotite-afa"                                                 },
	{ ".afm"           , APPLICATION "x-font-adobe-metric"                                             },
	{ ".ai"            , APPLICATION "illustrator"                                                     },
	{ ".aif"           , AUDIO "x-aiff"                                                                },
	{ ".aifc"          , AUDIO "x-aiff"                                                                },
	{ ".aiff"          , AUDIO "x-aiff"                                                                },
	{ ".air"           , APPLICATION "vnd.adobe.air-application-installer-package+zip;version=\"1.0\"" },
	{ ".aj"            , TEXT "x-aspectj"                                                              },
	{ ".amr"           , AUDIO "amr"                                                                   },
	{ ".anim"          , TEXT "x-yaml"                                                                 },
	{ ".anpa"          , TEXT "vnd.iptc.anpa"                                                          },
	{ ".ans"           , TEXT "plain"                                                                  },
	{ ".apl"           , TEXT "apl"                                                                    },
	{ ".apng"          , IMAGE "vnd.mozilla.apng"                                                      },
	{ ".applescript"   , TEXT "x-applescript"                                                          },
	{ ".apr"           , APPLICATION "vnd.lotus-approach;version=\"97\""                               },
	{ ".apt"           , APPLICATION "vnd.lotus-approach"                                              },
	{ ".arc"           , APPLICATION "x-internet-archive;version=\"1.0\""                              },
	{ ".arw"           , IMAGE "x-raw-sony"                                                            },
	{ ".as"            , TEXT "x-actionscript"                                                         },
	{ ".asc"           , TEXT "plain"                                                                  },
	{ ".asciidoc"      , TEXT "x-asciidoc"                                                             },
	{ ".asf"           , APPLICATION "vnd.ms-asf"                                                      },
	{ ".asice"         , APPLICATION "vnd.etsi.asic-e+zip"                                             },
	{ ".asics"         , APPLICATION "vnd.etsi.asic-s+zip"                                             },
	{ ".asn"           , TEXT "x-ttcn-asn"                                                             },
	{ ".asp"           , APPLICATION "x-aspx"                                                          },
	{ ".aspx"          , TEXT "aspdotnet"                                                              },
	{ ".asy"           , TEXT "x-spreadsheet"                                                          },
	{ ".atom"          , APPLICATION "atom+xml"                                                        },
	{ ".au"            , AUDIO "basic"                                                                 },
	{ ".avi"           , VIDEO "msvideo"                                                               },
	{ ".awb"           , AUDIO "amr-wb"                                                                },
	{ ".awk"           , TEXT "x-awk"                                                                  },
	{ ".axc"           , TEXT "plain"                                                                  },
	{ ".axx"           , APPLICATION "x-axcrypt"                                                       },
	{ ".b"             , TEXT "x-brainfuck"                                                            },
	{ ".b6z"           , APPLICATION "x-b6z-compressed"                                                },
	{ ".bas"           , TEXT "x-basic"                                                                },
	{ ".bay"           , IMAGE "x-raw-casio"                                                           },
	{ ".bik"           , VIDEO "vnd.radgamettools.bink"                                                },
	{ ".bik2"          , VIDEO "vnd.radgamettools.bink;version=\"2\""                                  },
	{ ".bin"           , APPLICATION "x-macbinary"                                                     },
	{ ".bmp"           , IMAGE "bmp"                                                                   },
	{ ".bpg"           , IMAGE "x-bpg"                                                                 },
	{ ".bpm"           , APPLICATION "bizagi-modeler"                                                  },
	{ ".bsp"           , MODEL "vnd.valve.source.compiled-map"                                         },
	{ ".btf"           , IMAGE "tiff"                                                                  },
	{ ".bw"            , IMAGE "x-sgi-bw"                                                              },
	{ ".bz2"           , APPLICATION "x-bzip2"                                                         },
	{ ".c"             , TEXT "x-csrc"                                                                 },
	{ ".cab"           , APPLICATION "vnd.ms-cab-compressed"                                           },
	{ ".cabal"         , TEXT "x-haskell"                                                              },
	{ ".cap"           , APPLICATION "vnd.tcpdump.pcap"                                                },
	{ ".catmaterial"   , APPLICATION "octet-stream;version=\"5\""                                      },
	{ ".catpart"       , APPLICATION "octet-stream;version=\"5\""                                      },
	{ ".catproduct"    , APPLICATION "octet-stream;version=\"5\""                                      },
	{ ".cbl"           , TEXT "x-cobol"                                                                },
	{ ".cbor"          , APPLICATION "cbor"                                                            },
	{ ".cbz"           , APPLICATION "x-cbr"                                                           },
	{ ".cca"           , APPLICATION "octet-stream"                                                    },
	{ ".cda"           , APPLICATION "x-cdf"                                                           },
	{ ".cdf"           , APPLICATION "x-netcdf"                                                        },
	{ ".cdx"           , CHEMICAL "x-cdx"                                                              },
	{ ".cfg"           , TEXT "x-config"                                                               },
	{ ".cfm"           , TEXT "x-coldfusion"                                                           },
	{ ".cfs"           , APPLICATION "x-cfs-compressed"                                                },
	{ ".cgi"           , TEXT "x-cgi"                                                                  },
	{ ".cgm"           , IMAGE "cgm"                                                                   },
	{ ".chm"           , APPLICATION "vnd.ms-htmlhelp"                                                 },
	{ ".chrt"          , APPLICATION "x-kchart"                                                        },
	{ ".chs"           , TEXT "x-haskell"                                                              },
	{ ".cif"           , CHEMICAL "x-cif"                                                              },
	{ ".ck"            , TEXT "x-java"                                                                 },
	{ ".cl"            , TEXT "x-common-lisp"                                                          },
	{ ".class"         , APPLICATION "x-java"                                                          },
	{ ".clj"           , TEXT "x-clojure"                                                              },
	{ ".cls"           , TEXT "x-vbasic"                                                               },
	{ ".cmake"         , TEXT "x-cmake"                                                                },
	{ ".cml"           , CHEMICAL "x-cml"                                                              },
	{ ".cob"           , TEXT "x-cobol"                                                                },
	{ ".cod"           , IMAGE "cis-cod"                                                               },
	{ ".coffee"        , APPLICATION "vnd.coffeescript"                                                },
	{ ".cp"            , TEXT "x-pascal"                                                               },
	{ ".cpio"          , APPLICATION "x-cpio"                                                          },
	{ ".cpp"           , TEXT "x-c++src"                                                               },
	{ ".cr"            , TEXT "x-crystal"                                                              },
	{ ".crw"           , IMAGE "x-canon-crw"                                                           },
	{ ".crx"           , APPLICATION "x-chrome-package"                                                },
	{ ".cs"            , TEXT "x-csharp"                                                               },
	{ ".cshtml"        , TEXT "html"                                                                   },
	{ ".cson"          , TEXT "x-coffeescript"                                                         },
	{ ".csr"           , APPLICATION "pkcs10"                                                          },
	{ ".css"           , TEXT "css"                                                                    },
	{ ".csv"           , TEXT "csv"                                                                    },
	{ ".csvs"          , TEXT "csv-schema"                                                             },
	{ ".cu"            , TEXT "x-c++src"                                                               },
	{ ".cur"           , IMAGE "x-win-bitmap"                                                          },
	{ ".cwl"           , TEXT "x-yaml"                                                                 },
	{ ".cy"            , TEXT "javascript"                                                             },
	{ ".d"             , TEXT "x-d"                                                                    },
	{ ".dae"           , TEXT "xml"                                                                    },
	{ ".dart"          , APPLICATION "dart"                                                            },
	{ ".dat"           , APPLICATION "dbase"                                                           },
	{ ".db"            , APPLICATION "x-sqlite3"                                                       },
	{ ".dbf"           , APPLICATION "x-dbf"                                                           },
	{ ".dcm"           , APPLICATION "dicom"                                                           },
	{ ".dcr"           , APPLICATION "x-director"                                                      },
	{ ".dcx"           , IMAGE "x-dcx"                                                                 },
	{ ".deb"           , APPLICATION "vnd.debian.binary-package"                                       },
	{ ".der"           , APPLICATION "x-x509-ca-cert"                                                  },
	{ ".dex"           , APPLICATION "x-dex"                                                           },
	{ ".dgc"           , APPLICATION "x-dgc-compressed"                                                },
	{ ".diff"          , TEXT "x-diff"                                                                 },
	{ ".dir"           , APPLICATION "x-director"                                                      },
	{ ".dita"          , APPLICATION "dita+xml;format=topic"                                           },
	{ ".ditamap"       , APPLICATION "dita+xml;format=map"                                             },
	{ ".ditaval"       , APPLICATION "dita+xml;format=val"                                             },
	{ ".diz"           , TEXT "plain"                                                                  },
	{ ".djv"           , IMAGE "vnd.djvu"                                                              },
	{ ".djvu"          , IMAGE "vnd.djvu"                                                              },
	{ ".dll"           , APPLICATION "octet-stream"                                                    },
	{ ".dls"           , AUDIO "dls"                                                                   },
	{ ".dmg"           , APPLICATION "x-apple-diskimage"                                               },
	{ ".dng"           , IMAGE "x-raw-adobe"                                                           },
	{ ".doc"           , APPLICATION "msword"                                                          },
	{ ".dockerfile"    , TEXT "x-dockerfile"                                                           },
	{ ".docm"          , APPLICATION "vnd.ms-word.document.macroenabled.12"                            },
	{ ".docx"          , APPLICATION "vnd.openxmlformats-officedocument.wordprocessingml.document"     },
	{ ".dot"           , APPLICATION "msword;version=\"97-2003\""                                      },
	{ ".dotm"          , APPLICATION "vnd.ms-word.template.macroenabled.12"                            },
	{ ".dotx"          , APPLICATION "vnd.openxmlformats-officedocument.wordprocessingml.template"     },
	{ ".dpx"           , IMAGE "x-dpx"                                                                 },
	{ ".drc"           , VIDEO "x-dirac"                                                               },
	{ ".druby"         , TEXT "x-ruby"                                                                 },
	{ ".dtd"           , APPLICATION "xml-dtd"                                                         },
	{ ".dts"           , AUDIO "vnd.dts"                                                               },
	{ ".dv"            , VIDEO "dv"                                                                    },
	{ ".dvi"           , APPLICATION "x-dvi"                                                           },
	{ ".dwf"           , APPLICATION "dwf;version=\"6.0\""                                             },
	{ ".dwfx"          , MODEL "vnd.dwfx+xps"                                                          },
	{ ".dwg"           , IMAGE "vnd.dwg"                                                               },
	{ ".dwl"           , APPLICATION "octet-stream"                                                    },
	{ ".dx"            , APPLICATION "dec-dx."                                                         },
	{ ".dxb"           , IMAGE "vnd.dxb"                                                               },
	{ ".dxf"           , IMAGE "vnd.dxf"                                                               },
	{ ".dxr"           , AUDIO "prs.sid;version=\"1\""                                                 },
	{ ".dylan"         , TEXT "x-dylan"                                                                },
	{ ".e"             , TEXT "x-eiffel"                                                               },
	{ ".eb"            , TEXT "x-python"                                                               },
	{ ".ebnf"          , TEXT "x-ebnf"                                                                 },
	{ ".ebuild"        , TEXT "x-sh"                                                                   },
	{ ".ecl"           , TEXT "x-ecl"                                                                  },
	{ ".eclass"        , TEXT "x-sh"                                                                   },
	{ ".ecr"           , TEXT "html"                                                                   },
	{ ".edc"           , APPLICATION "json"                                                            },
	{ ".edn"           , TEXT "x-clojure"                                                              },
	{ ".eex"           , TEXT "html"                                                                   },
	{ ".el"            , TEXT "x-emacs-lisp"                                                           },
	{ ".elc"           , APPLICATION "x-elc"                                                           },
	{ ".elm"           , TEXT "x-elm"                                                                  },
	{ ".em"            , TEXT "x-coffeescript"                                                         },
	{ ".emf"           , IMAGE "emf"                                                                   },
	{ ".eml"           , MESSAGE "rfc822"                                                              },
	{ ".enr"           , APPLICATION "x-endnote-refer"                                                 },
	{ ".ens"           , APPLICATION "x-endnote-style"                                                 },
	{ ".enz"           , APPLICATION "x-endnote-connect"                                               },
	{ ".eot"           , APPLICATION "vnd.ms-fontobject"                                               },
	{ ".epj"           , APPLICATION "json"                                                            },
	{ ".eps"           , APPLICATION "postscript"                                                      },
	{ ".epub"          , APPLICATION "epub+zip"                                                        },
	{ ".eq"            , TEXT "x-csharp"                                                               },
	{ ".erb"           , APPLICATION "x-erb"                                                           },
	{ ".erf"           , IMAGE "x-raw-epson"                                                           },
	{ ".erl"           , TEXT "x-erlang"                                                               },
	{ ".es"            , APPLICATION "ecmascript"                                                      },
	{ ".exe"           , APPLICATION "x-dosexec"                                                       },
	{ ".exp"           , TEXT "x-expect"                                                               },
	{ ".exr"           , IMAGE "x-exr;version=\"2\""                                                   },
	{ ".f"             , TEXT "x-fortran"                                                              },
	{ ".f4a"           , VIDEO "mp4"                                                                   },
	{ ".f4m"           , APPLICATION "f4m"                                                             },
	{ ".f90"           , TEXT "x-fortran"                                                              },
	{ ".factor"        , TEXT "x-factor"                                                               },
	{ ".fb2"           , APPLICATION "x-fictionbook+xml"                                               },
	{ ".fdf"           , APPLICATION "vnd.fdf"                                                         },
	{ ".fff"           , IMAGE "x-raw-imacon"                                                          },
	{ ".fh"            , IMAGE "x-freehand"                                                            },
	{ ".fig"           , APPLICATION "x-matlab-data"                                                   },
	{ ".fits"          , IMAGE "fits"                                                                  },
	{ ".flac"          , AUDIO "flac"                                                                  },
	{ ".flif"          , IMAGE "flif"                                                                  },
	{ ".flv"           , VIDEO "x-flv"                                                                 },
	{ ".fm"            , APPLICATION "vnd.framemaker"                                                  },
	{ ".fmz"           , APPLICATION "octet-stream"                                                    },
	{ ".folio"         , APPLICATION "vnd.adobe.folio+zip"                                             },
	{ ".fp7"           , APPLICATION "x-filemaker"                                                     },
	{ ".fpx"           , IMAGE "vnd.fpx"                                                               },
	{ ".fref"          , TEXT "plain"                                                                  },
	{ ".fs"            , TEXT "x-fsharp"                                                               },
	{ ".fth"           , TEXT "x-forth"                                                                },
	{ ".g3"            , IMAGE "g3fax"                                                                 },
	{ ".gbr"           , APPLICATION "vnd.gerber"                                                      },
	{ ".gca"           , APPLICATION "x-gca-compressed"                                                },
	{ ".geo"           , APPLICATION "vnd.geogebra.file;version=\"1.0\""                               },
	{ ".gf"            , TEXT "x-haskell"                                                              },
	{ ".ggb"           , APPLICATION "vnd.geogebra.file"                                               },
	{ ".gif"           , IMAGE "gif"                                                                   },
	{ ".gitconfig"     , TEXT "x-properties"                                                           },
	{ ".gitignore"     , TEXT "x-sh"                                                                   },
	{ ".glf"           , TEXT "x-tcl"                                                                  },
	{ ".gml"           , APPLICATION "gml+xml"                                                         },
	{ ".gn"            , TEXT "x-python"                                                               },
	{ ".gnumeric"      , APPLICATION "x-gnumeric"                                                      },
	{ ".go"            , TEXT "x-go"                                                                   },
	{ ".grb"           , APPLICATION "x-grib"                                                          },
	{ ".groovy"        , TEXT "x-groovy"                                                               },
	{ ".gsp"           , APPLICATION "x-jsp"                                                           },
	{ ".gtar"          , APPLICATION "x-gtar"                                                          },
	{ ".gz"            , APPLICATION "gzip"                                                            },
	{ ".h"             , TEXT "x-chdr"                                                                 },
	{ ".haml"          , TEXT "x-haml"                                                                 },
	{ ".hcl"           , TEXT "x-ruby"                                                                 },
	{ ".hdf"           , APPLICATION "x-hdf"                                                           },
	{ ".hdp"           , IMAGE "vnd.ms-photo"                                                          },
	{ ".hdr"           , IMAGE "vnd.radiance"                                                          },
	{ ".hh"            , APPLICATION "x-httpd-php"                                                     },
	{ ".hpgl"          , APPLICATION "vnd.hp-hpgl"                                                     },
	{ ".hpp"           , TEXT "x-c++hdr"                                                               },
	{ ".hqx"           , APPLICATION "mac-binhex"                                                      },
	{ ".hs"            , TEXT "x-haskell"                                                              },
	{ ".htm"           , TEXT "html"                                                                   },
	{ ".html"          , TEXT "html"                                                                   },
	{ ".http"          , MESSAGE "http"                                                                },
	{ ".hx"            , TEXT "x-haxe"                                                                 },
	{ ".ibooks"        , APPLICATION "x-ibooks+zip"                                                    },
	{ ".ico"           , IMAGE "vnd.microsoft.icon"                                                    },
	{ ".ics"           , TEXT "calendar"                                                               },
	{ ".idl"           , TEXT "x-idl"                                                                  },
	{ ".idml"          , APPLICATION "vnd.adobe.indesign-idml-package"                                 },
	{ ".iges"          , MODEL "iges;version=\"5.x\""                                                  },
	{ ".igs"           , MODEL "iges"                                                                  },
	{ ".iiq"           , IMAGE "x-raw-phaseone"                                                        },
	{ ".ind"           , APPLICATION "octet-stream;version=\"generic\""                                },
	{ ".indd"          , APPLICATION "x-adobe-indesign"                                                },
	{ ".inf"           , APPLICATION "inf"                                                             },
	{ ".ini"           , TEXT "x-ini"                                                                  },
	{ ".inx"           , APPLICATION "x-adobe-indesign-interchange"                                    },
	{ ".ipa"           , APPLICATION "x-itunes-ipa"                                                    },
	{ ".ipynb"         , APPLICATION "json"                                                            },
	{ ".irclog"        , TEXT "mirc"                                                                   },
	{ ".iso"           , APPLICATION "x-iso9660-image"                                                 },
	{ ".itk"           , APPLICATION "x-tcl"                                                           },
	{ ".j2c"           , IMAGE "x-jp2-codestream"                                                      },
	{ ".jade"          , TEXT "x-pug"                                                                  },
	{ ".jar"           , APPLICATION "java-archive"                                                    },
	{ ".java"          , TEXT "x-java"                                                                 },
	{ ".jinja"         , TEXT "x-django"                                                               },
	{ ".jl"            , TEXT "x-julia"                                                                },
	{ ".jls"           , IMAGE "jls"                                                                   },
	{ ".jng"           , IMAGE "x-jng"                                                                 },
	{ ".jnilib"        , APPLICATION "x-java-jnilib"                                                   },
	{ ".jp2"           , IMAGE "jpx"                                                                   },
	{ ".jpe"           , IMAGE "jpeg"                                                                  },
	{ ".jpf"           , IMAGE "jpx"                                                                   },
	{ ".jpg"           , IMAGE "jpeg"                                                                  },
	{ ".jpm"           , IMAGE "jpm"                                                                   },
	{ ".jq"            , APPLICATION "json"                                                            },
	{ ".js"            , APPLICATION "javascript"                                                      },
	{ ".json"          , APPLICATION "json"                                                            },
	{ ".json-patch"    , APPLICATION "json-patch+json"                                                 },
	{ ".json5"         , APPLICATION "json"                                                            },
	{ ".jsonld"        , APPLICATION "ld+json"                                                         },
	{ ".jsp"           , APPLICATION "x-jsp"                                                           },
	{ ".jsx"           , TEXT "jsx"                                                                    },
	{ ".jxr"           , IMAGE "jxr"                                                                   },
	{ ".k25"           , IMAGE "x-raw-kodak"                                                           },
	{ ".kicad_pcb"     , TEXT "x-common-lisp"                                                          },
	{ ".kid"           , TEXT "xml"                                                                    },
	{ ".kil"           , APPLICATION "x-killustrator"                                                  },
	{ ".kit"           , TEXT "html"                                                                   },
	{ ".kml"           , APPLICATION "vnd.google-earth.kml+xml"                                        },
	{ ".kmz"           , APPLICATION "vnd.google-earth.kmz"                                            },
	{ ".kpr"           , APPLICATION "x-kpresenter"                                                    },
	{ ".kra"           , APPLICATION "x-krita"                                                         },
	{ ".ksp"           , APPLICATION "x-kspread"                                                       },
	{ ".kt"            , TEXT "x-kotlin"                                                               },
	{ ".ktx"           , IMAGE "ktx"                                                                   },
	{ ".kwd"           , APPLICATION "x-kword"                                                         },
	{ ".l"             , TEXT "x-lex"                                                                  },
	{ ".latex"         , APPLICATION "x-latex"                                                         },
	{ ".latte"         , TEXT "x-smarty"                                                               },
	{ ".less"          , TEXT "x-less"                                                                 },
	{ ".lfe"           , TEXT "x-common-lisp"                                                          },
	{ ".lha"           , APPLICATION "x-lzh-compressed"                                                },
	{ ".lhs"           , TEXT "x-literate-haskell"                                                     },
	{ ".lisp"          , TEXT "x-common-lisp"                                                          },
	{ ".log"           , TEXT "x-log"                                                                  },
	{ ".lookml"        , TEXT "x-yaml"                                                                 },
	{ ".ls"            , TEXT "x-livescript"                                                           },
	{ ".lua"           , TEXT "x-lua"                                                                  },
	{ ".lvproj"        , TEXT "xml"                                                                    },
	{ ".lwp"           , APPLICATION "vnd.lotus-wordpro"                                               },
	{ ".lz"            , APPLICATION "x-lzip"                                                          },
	{ ".lzma"          , APPLICATION "x-lzma"                                                          },
	{ ".m"             , TEXT "x-objcsrc"                                                              },
	{ ".m3"            , TEXT "x-modula"                                                               },
	{ ".m3u"           , AUDIO "x-mpegurl"                                                             },
	{ ".maff"          , APPLICATION "x-maff"                                                          },
	{ ".mak"           , TEXT "x-cmake"                                                                },
	{ ".marko"         , TEXT "html"                                                                   },
	{ ".mas"           , APPLICATION "vnd.lotus-freelance"                                             },
	{ ".mat"           , APPLICATION "matlab-mat"                                                      },
	{ ".mathematica"   , TEXT "x-mathematica"                                                          },
	{ ".matlab"        , TEXT "x-octave"                                                               },
	{ ".maxpat"        , APPLICATION "json"                                                            },
	{ ".mbox"          , APPLICATION "mbox"                                                            },
	{ ".mbx"           , APPLICATION "mbox"                                                            },
	{ ".mcw"           , APPLICATION "msword;version=\"5.0\""                                          },
	{ ".md"            , TEXT "markdown"                                                               },
	{ ".mdi"           , IMAGE "vnd.ms-modi"                                                           },
	{ ".mef"           , IMAGE "x-raw-mamiya"                                                          },
	{ ".metal"         , TEXT "x-c++src"                                                               },
	{ ".mht"           , MULTIPART "multipart/related"                                                           },
	{ ".mid"           , AUDIO "midi"                                                                  },
	{ ".mif"           , APPLICATION "x-mif"                                                           },
	{ ".mix"           , IMAGE "vnd.mix"                                                               },
	{ ".mj2"           , VIDEO "mj2"                                                                   },
	{ ".mkv"           , VIDEO "x-matroska"                                                            },
	{ ".ml"            , TEXT "x-ocaml"                                                                },
	{ ".mlp"           , AUDIO "vnd.dolby.mlp"                                                         },
	{ ".mm"            , APPLICATION "x-freemind"                                                      },
	{ ".mmp"           , APPLICATION "vnd.mindjet.mindmanager"                                         },
	{ ".mmr"           , IMAGE "vnd.fujixerox.edmics-mmr"                                              },
	{ ".mng"           , VIDEO "x-mng"                                                                 },
	{ ".mo"            , TEXT "x-modelica"                                                             },
	{ ".mobi"          , APPLICATION "x-mobipocket-ebook"                                              },
	{ ".mod"           , APPLICATION "octet-stream;version=\"4\""                                      },
	{ ".mol"           , CHEMICAL "x-mdl-molfile"                                                      },
	{ ".mos"           , IMAGE "x-raw-leaf"                                                            },
	{ ".mov"           , VIDEO "quicktime"                                                             },
	{ ".mp1"           , AUDIO "mpeg"                                                                  },
	{ ".mp2"           , AUDIO "mpeg"                                                                  },
	{ ".mp3"           , AUDIO "mpeg"                                                                  },
	{ ".mp4"           , VIDEO "mp4"                                                                   },
	{ ".mpeg"          , VIDEO "mpeg"                                                                  },
	{ ".mpg"           , VIDEO "mpeg"                                                                  },
	{ ".mpga"          , AUDIO "mpeg"                                                                  },
	{ ".mpp"           , APPLICATION "vnd.ms-project;version=\"2010\""                                 },
	{ ".mpx"           , APPLICATION "x-project;version=\"4.0\""                                       },
	{ ".mrc"           , APPLICATION "marc"                                                            },
	{ ".mrw"           , IMAGE "x-raw-minolta"                                                         },
	{ ".msg"           , APPLICATION "vnd.ms-outlook"                                                  },
	{ ".msi"           , APPLICATION "x-msi"                                                           },
	{ ".mso"           , APPLICATION "x-mso"                                                           },
	{ ".mtml"          , TEXT "html"                                                                   },
	{ ".muf"           , TEXT "x-forth"                                                                },
	{ ".mumps"         , TEXT "x-mumps"                                                                },
	{ ".mxf"           , APPLICATION "mxf"                                                             },
	{ ".mxl"           , APPLICATION "vnd.recordare.musicxml"                                          },
	{ ".mxmf"          , AUDIO "mobile-xmf"                                                            },
	{ ".n3"            , TEXT "n3"                                                                     },
	{ ".nap"           , IMAGE "naplps"                                                                },
	{ ".nb"            , APPLICATION "mathematica"                                                     },
	{ ".nc"            , APPLICATION "x-netcdf"                                                        },
	{ ".nef"           , IMAGE "x-raw-nikon"                                                           },
	{ ".nfo"           , TEXT "x-nfo"                                                                  },
	{ ".nginxconf"     , TEXT "x-nginx-conf"                                                           },
	{ ".nif"           , APPLICATION "vnd.music-niff"                                                  },
	{ ".nl"            , TEXT "x-common-lisp"                                                          },
	{ ".nlogo"         , TEXT "x-common-lisp"                                                          },
	{ ".ns2"           , APPLICATION "vnd.lotus-notes;version=\"2\""                                   },
	{ ".ns3"           , APPLICATION "vnd.lotus-notes;version=\"3\""                                   },
	{ ".ns4"           , APPLICATION "vnd.lotus-notes;version=\"4\""                                   },
	{ ".nsf"           , APPLICATION "vnd.lotus-notes"                                                 },
	{ ".nsi"           , TEXT "x-nsis"                                                                 },
	{ ".ntf"           , APPLICATION "vnd.nitf"                                                        },
	{ ".nu"            , TEXT "x-scheme"                                                               },
	{ ".numpy"         , TEXT "x-python"                                                               },
	{ ".nut"           , TEXT "x-c++src"                                                               },
	{ ".ocaml"         , TEXT "x-ocaml"                                                                },
	{ ".odb"           , APPLICATION "vnd.oasis.opendocument.database"                                 },
	{ ".odc"           , APPLICATION "vnd.oasis.opendocument.chart"                                    },
	{ ".odf"           , APPLICATION "vnd.oasis.opendocument.formula"                                  },
	{ ".odft"          , APPLICATION "vnd.oasis.opendocument.formula-template"                         },
	{ ".odg"           , APPLICATION "vnd.oasis.opendocument.graphics"                                 },
	{ ".odi"           , APPLICATION "vnd.oasis.opendocument.image"                                    },
	{ ".odm"           , APPLICATION "vnd.oasis.opendocument.text-master"                              },
	{ ".odp"           , APPLICATION "vnd.oasis.opendocument.presentation"                             },
	{ ".ods"           , APPLICATION "vnd.oasis.opendocument.spreadsheet"                              },
	{ ".odt"           , APPLICATION "vnd.oasis.opendocument.text"                                     },
	{ ".ofx"           , APPLICATION "x-ofx;version=\"2.1.1\""                                         },
	{ ".oga"           , AUDIO "ogg"                                                                   },
	{ ".ogg"           , AUDIO "ogg"                                                                   },
	{ ".ogm"           , VIDEO "x-ogm"                                                                 },
	{ ".ogv"           , VIDEO "ogg"                                                                   },
	{ ".ogx"           , APPLICATION "ogg"                                                             },
	{ ".one"           , APPLICATION "msonenote"                                                       },
	{ ".opf"           , APPLICATION "x-dtbook+xml"                                                    },
	{ ".opus"          , AUDIO "opus"                                                                  },
	{ ".ora"           , IMAGE "openraster"                                                            },
	{ ".orf"           , IMAGE "x-raw-olympus"                                                         },
	{ ".otc"           , APPLICATION "vnd.oasis.opendocument.chart-template"                           },
	{ ".otf"           , APPLICATION "x-font-otf"                                                      },
	{ ".otg"           , APPLICATION "vnd.oasis.opendocument.graphics-template"                        },
	{ ".oth"           , APPLICATION "vnd.oasis.opendocument.text-web"                                 },
	{ ".oti"           , APPLICATION "vnd.oasis.opendocument.image-template"                           },
	{ ".otm"           , APPLICATION "vnd.oasis.opendocument.text-master"                              },
	{ ".otp"           , APPLICATION "vnd.oasis.opendocument.presentation-template"                    },
	{ ".ots"           , APPLICATION "vnd.oasis.opendocument.spreadsheet-template"                     },
	{ ".ott"           , APPLICATION "vnd.oasis.opendocument.text-template"                            },
	{ ".ova"           , APPLICATION "x-ova"                                                           },
	{ ".oxps"          , APPLICATION "oxps"                                                            },
	{ ".oz"            , TEXT "x-oz"                                                                   },
	{ ".p"             , TEXT "x-pascal"                                                               },
	{ ".p65"           , APPLICATION "vnd.pagemaker"                                                   },
	{ ".pab"           , APPLICATION "vnd.ms-outlook"                                                  },
	{ ".pack"          , APPLICATION "package"                                                         },
	{ ".pam"           , IMAGE "x-portable-arbitrarymap"                                               },
	{ ".pas"           , TEXT "x-pascal"                                                               },
	{ ".pbm"           , IMAGE "x-portable-bitmap"                                                     },
	{ ".pcap"          , APPLICATION "vnd.tcpdump.pcap"                                                },
	{ ".pcapng"        , APPLICATION "vnd.tcpdump.pcap"                                                },
	{ ".pcl"           , APPLICATION "vnd.hp-pcl"                                                      },
	{ ".pct"           , IMAGE "x-pict;version=\"2.0\""                                                },
	{ ".pcx"           , IMAGE "vnd.zbrush.pcx"                                                        },
	{ ".pdb"           , APPLICATION "vnd.palm"                                                        },
	{ ".pdf"           , APPLICATION "pdf"                                                             },
	{ ".pfm"           , APPLICATION "x-font-printer-metric"                                           },
	{ ".pfr"           , APPLICATION "font-tdpfr"                                                      },
	{ ".pgm"           , IMAGE "x-portable-graymap"                                                    },
	{ ".pgn"           , APPLICATION "x-chess-pgn"                                                     },
	{ ".pgsql"         , TEXT "x-sql"                                                                  },
	{ ".php"           , TEXT "x-php"                                                                  },
	{ ".phtml"         , APPLICATION "x-httpd-php"                                                     },
	{ ".pic"           , IMAGE "x-pict"                                                                },
	{ ".pict"          , IMAGE "x-pict"                                                                },
	{ ".pkpass"        , APPLICATION "vnd.apple.pkpass"                                                },
	{ ".pl"            , TEXT "x-perl"                                                                 },
	{ ".pls"           , TEXT "x-plsql"                                                                },
	{ ".png"           , IMAGE "png"                                                                   },
	{ ".pnm"           , IMAGE "x-portable-anymap"                                                     },
	{ ".pod"           , TEXT "x-perl"                                                                 },
	{ ".por"           , APPLICATION "x-spss-por"                                                      },
	{ ".potm"          , APPLICATION "vnd.ms-powerpoint.template.macroenabled.12;version=\"2007\""     },
	{ ".potx"          , APPLICATION "vnd.openxmlformats-officedocument.presentationml.template"       },
	{ ".pp"            , TEXT "x-puppet"                                                               },
	{ ".ppam"          , APPLICATION "vnd.ms-powerpoint.addin.macroenabled.12"                         },
	{ ".ppm"           , IMAGE "x-portable-pixmap"                                                     },
	{ ".pps"           , APPLICATION "vnd.ms-powerpoint;version=\"97-2003\""                           },
	{ ".ppsm"          , APPLICATION "vnd.ms-powerpoint.slideshow.macroenabled.12"                     },
	{ ".ppsx"          , APPLICATION "vnd.openxmlformats-officedocument.presentationml.slideshow"      },
	{ ".ppt"           , APPLICATION "vnd.ms-powerpoint"                                               },
	{ ".pptm"          , APPLICATION "vnd.ms-powerpoint.presentation.macroenabled.12"                  },
	{ ".pptx"          , APPLICATION "vnd.openxmlformats-officedocument.presentationml.presentation"   },
	{ ".prc"           , APPLICATION "vnd.palm"                                                        },
	{ ".pro"           , TEXT "x-prolog"                                                               },
	{ ".project"       , APPLICATION "octet-stream;version=\"4\""                                      },
	{ ".properties"    , TEXT "properties"                                                             },
	{ ".proto"         , TEXT "x-protobuf"                                                             },
	{ ".ps"            , APPLICATION "postscript"                                                      },
	{ ".ps1"           , APPLICATION "x-powershell"                                                    },
	{ ".psb"           , IMAGE "vnd.adobe.photoshop"                                                   },
	{ ".psd"           , IMAGE "vnd.adobe.photoshop"                                                   },
	{ ".psid"          , AUDIO "prs.sid;version=\"2\""                                                 },
	{ ".pst"           , APPLICATION "vnd.ms-outlook-pst"                                              },
	{ ".ptx"           , IMAGE "x-raw-pentax"                                                          },
	{ ".purs"          , TEXT "x-haskell"                                                              },
	{ ".pxn"           , IMAGE "x-raw-logitech"                                                        },
	{ ".py"            , TEXT "x-python"                                                               },
	{ ".pyx"           , TEXT "x-cython"                                                               },
	{ ".qcd"           , APPLICATION "vnd.quark.quarkxpress"                                           },
	{ ".qcp"           , AUDIO "qcelp"                                                                 },
	{ ".qif"           , APPLICATION "qif"                                                             },
	{ ".qxp"           , APPLICATION "vnd.quark.quarkxpress"                                           },
	{ ".qxp report"    , APPLICATION "vnd.quark.quarkxpress"                                           },
	{ ".r"             , TEXT "x-rsrc"                                                                 },
	{ ".r3d"           , IMAGE "x-raw-red"                                                             },
	{ ".ra"            , AUDIO "vnd.rn-realaudio"                                                      },
	{ ".raf"           , IMAGE "x-raw-fuji"                                                            },
	{ ".ram"           , AUDIO "vnd.rn-realaudio"                                                      },
	{ ".raml"          , TEXT "x-yaml"                                                                 },
	{ ".rar"           , APPLICATION "vnd.rar"                                                         },
	{ ".rar"           , APPLICATION "vnd.rar"                                                         },
	{ ".ras"           , IMAGE "x-sun-raster"                                                          },
	{ ".raw"           , IMAGE "x-raw-panasonic"                                                       },
	{ ".rb"            , TEXT "x-ruby"                                                                 },
	{ ".rdf"           , APPLICATION "rdf+xml"                                                         },
	{ ".re"            , TEXT "x-rustsrc"                                                              },
	{ ".reg"           , TEXT "x-properties"                                                           },
	{ ".rest"          , TEXT "x-rst"                                                                  },
	{ ".rexx"          , TEXT "x-rexx"                                                                 },
	{ ".rf64"          , AUDIO "x-wav;version=\"0waveformatextensibleencoding\""                       },
	{ ".rfa"           , APPLICATION "octet-stream"                                                    },
	{ ".rft"           , APPLICATION "octet-stream"                                                    },
	{ ".rg"            , TEXT "x-clojure"                                                              },
	{ ".rgb"           , IMAGE "x-rgb"                                                                 },
	{ ".rhtml"         , APPLICATION "x-erb"                                                           },
	{ ".rlc"           , IMAGE "vnd.fujixerox.edmics-rlc"                                              },
	{ ".rm"            , APPLICATION "vnd.rn-realmedia"                                                },
	{ ".rmd"           , TEXT "x-gfm"                                                                  },
	{ ".rmi"           , AUDIO "mid"                                                                   },
	{ ".rmp"           , AUDIO "x-pn-realaudio-plugin"                                                 },
	{ ".roff"          , TEXT "troff"                                                                  },
	{ ".rpm"           , APPLICATION "x-rpm"                                                           },
	{ ".rs"            , TEXT "x-rustsrc"                                                              },
	{ ".rss"           , APPLICATION "rss+xml"                                                         },
	{ ".rst"           , TEXT "x-rst"                                                                  },
	{ ".rte"           , APPLICATION "octet-stream"                                                    },
	{ ".rtf"           , APPLICATION "rtf"                                                             },
	{ ".rv"            , VIDEO "vnd.rn-realvideo"                                                      },
	{ ".rvg"           , APPLICATION "octet-stream"                                                    },
	{ ".rvt"           , APPLICATION "octet-stream"                                                    },
	{ ".rws"           , APPLICATION "octet-stream"                                                    },
	{ ".rwz"           , IMAGE "x-raw-rawzor"                                                          },
	{ ".s"             , TEXT "x-asm"                                                                  },
	{ ".s7m"           , APPLICATION "x-sas-dmdb"                                                      },
	{ ".sa7"           , APPLICATION "x-sas-access"                                                    },
	{ ".sage"          , TEXT "x-python"                                                               },
	{ ".sam"           , APPLICATION "vnd.lotus-wordpro"                                               },
	{ ".sas"           , APPLICATION "x-sas"                                                           },
	{ ".sas7bbak"      , APPLICATION "x-sas-backup"                                                    },
	{ ".sass"          , TEXT "x-sass"                                                                 },
	{ ".sav"           , APPLICATION "x-spss-sav"                                                      },
	{ ".sc7"           , APPLICATION "x-sas-catalog"                                                   },
	{ ".scala"         , TEXT "x-scala"                                                                },
	{ ".sch"           , TEXT "xml"                                                                    },
	{ ".scm"           , TEXT "x-scheme"                                                               },
	{ ".scores"        , TEXT "plain"                                                                  },
	{ ".scss"          , TEXT "x-scss"                                                                 },
	{ ".sd7"           , APPLICATION "x-sas-data"                                                      },
	{ ".sda"           , APPLICATION "vnd.stardivision.draw"                                           },
	{ ".sdc"           , APPLICATION "vnd.stardivision.calc"                                           },
	{ ".sdn"           , TEXT "plain"                                                                  },
	{ ".sdw"           , APPLICATION "vnd.stardivision.writer"                                         },
	{ ".sed"           , TEXT "x-sed"                                                                  },
	{ ".sf7"           , APPLICATION "x-sas-fdb"                                                       },
	{ ".sfd"           , APPLICATION "vnd.font-fontforge-sfd"                                          },
	{ ".sgm"           , TEXT "sgml"                                                                   },
	{ ".sgml"          , TEXT "sgml"                                                                   },
	{ ".sh"            , APPLICATION "x-sh"                                                            },
	{ ".sh-session"    , TEXT "x-sh"                                                                   },
	{ ".shar"          , APPLICATION "x-shar"                                                          },
	{ ".shtml"         , TEXT "x-server-parsed-html"                                                   },
	{ ".si7"           , APPLICATION "x-sas-data-index"                                                },
	{ ".sid"           , AUDIO "prs.sid"                                                               },
	{ ".sit"           , APPLICATION "x-stuffit"                                                       },
	{ ".sitx"          , APPLICATION "x-stuffitx"                                                      },
	{ ".skb"           , APPLICATION "octet-stream"                                                    },
	{ ".skp"           , APPLICATION "vnd.koan"                                                        },
	{ ".sla"           , APPLICATION "vnd.scribus"                                                     },
	{ ".sld"           , APPLICATION "sld"                                                             },
	{ ".sldm"          , APPLICATION "vnd.ms-powerpoint.slide.macroenabled.12;version=\"2007\""        },
	{ ".sldprt"        , APPLICATION "sldworks"                                                        },
	{ ".slim"          , TEXT "x-slim"                                                                 },
	{ ".sls"           , TEXT "x-yaml"                                                                 },
	{ ".sm7"           , APPLICATION "x-sas-mddb"                                                      },
	{ ".smi"           , APPLICATION "smil+xml"                                                        },
	{ ".smk"           , VIDEO "vnd.radgamettools.smacker"                                             },
	{ ".soy"           , TEXT "x-soy"                                                                  },
	{ ".sp7"           , APPLICATION "x-sas-putility"                                                  },
	{ ".sparql"        , APPLICATION "sparql-query"                                                    },
	{ ".spec"          , TEXT "x-rpm-spec"                                                             },
	{ ".spl"           , APPLICATION "x-futuresplash"                                                  },
	{ ".spx"           , AUDIO "speex"                                                                 },
	{ ".sql"           , TEXT "x-sql"                                                                  },
	{ ".sr7"           , APPLICATION "x-sas-itemstor"                                                  },
	{ ".srl"           , APPLICATION "sereal"                                                          },
	{ ".srt"           , TEXT "x-common-lisp"                                                          },
	{ ".ss7"           , APPLICATION "x-sas-program-data"                                              },
	{ ".ssml"          , APPLICATION "ssml+xml"                                                        },
	{ ".st"            , TEXT "x-stsrc"                                                                },
	{ ".st7"           , APPLICATION "x-sas-audit"                                                     },
	{ ".stw"           , APPLICATION "vnd.sun.xml.writer.template"                                     },
	{ ".stx"           , APPLICATION "x-sas-transport"                                                 },
	{ ".su7"           , APPLICATION "x-sas-utility"                                                   },
	{ ".sublime-build" , TEXT "javascript"                                                             },
	{ ".sv"            , TEXT "x-systemverilog"                                                        },
	{ ".sv7"           , APPLICATION "x-sas-view"                                                      },
	{ ".svf"           , IMAGE "vnd.svf"                                                               },
	{ ".svg"           , IMAGE "svg+xml"                                                               },
	{ ".svgz"          , IMAGE "svg+xml"                                                               },
	{ ".swf"           , APPLICATION "vnd.adobe.flash-movie"                                           },
	{ ".swift"         , TEXT "x-swift"                                                                },
	{ ".sxc"           , APPLICATION "vnd.sun.xml.calc;version=\"1.0\""                                },
	{ ".sxd"           , APPLICATION "vnd.sun.xml.draw"                                                },
	{ ".sxi"           , APPLICATION "vnd.sun.xml.impress"                                             },
	{ ".sxw"           , APPLICATION "vnd.sun.xml.writer"                                              },
	{ ".sz"            , APPLICATION "x-snappy-framed"                                                 },
	{ ".t"             , APPLICATION "x-tads"                                                          },
	{ ".tap"           , IMAGE "vnd.tencent.tap"                                                       },
	{ ".tar"           , APPLICATION "x-tar"                                                           },
	{ ".tcl"           , TEXT "x-tcl"                                                                  },
	{ ".tcsh"          , TEXT "x-sh"                                                                   },
	{ ".tex"           , TEXT "x-tex"                                                                  },
	{ ".textile"       , TEXT "x-textile"                                                              },
	{ ".tfw"           , TEXT "plain"                                                                  },
	{ ".tfx"           , IMAGE "tiff"                                                                  },
	{ ".thmx"          , APPLICATION "vnd.ms-officetheme"                                              },
	{ ".tif"           , IMAGE "tiff"                                                                  },
	{ ".tif "          , IMAGE "tiff"                                                                  },
	{ ".tiff"          , IMAGE "tiff"                                                                  },
	{ ".toml"          , TEXT "x-toml"                                                                 },
	{ ".torrent"       , APPLICATION "x-bittorrent"                                                    },
	{ ".tpl"           , TEXT "x-smarty"                                                               },
	{ ".ts"            , APPLICATION "typescript"                                                      },
	{ ".tsv"           , TEXT "tab-separated-values"                                                   },
	{ ".tta"           , AUDIO "tta;version=\"1\""                                                     },
	{ ".ttf"           , APPLICATION "x-font-ttf"                                                      },
	{ ".ttl"           , TEXT "turtle"                                                                 },
	{ ".twig"          , TEXT "x-twig"                                                                 },
	{ ".txt"           , TEXT "plain"                                                                  },
	{ ".u3d"           , MODEL "u3d"                                                                   },
	{ ".uc"            , TEXT "x-java"                                                                 },
	{ ".ulx"           , APPLICATION "x-glulx"                                                         },
	{ ".uno"           , TEXT "x-csharp"                                                               },
	{ ".upc"           , TEXT "x-csrc"                                                                 },
	{ ".url"           , APPLICATION "x-url"                                                           },
	{ ".v"             , TEXT "x-verilog"                                                              },
	{ ".vb"            , TEXT "x-vb"                                                                   },
	{ ".vbs"           , TEXT "x-vbscript"                                                             },
	{ ".vcd"           , APPLICATION "x-cdlink"                                                        },
	{ ".vcf"           , TEXT "vcard"                                                                  },
	{ ".vcs"           , TEXT "x-vcalendar"                                                            },
	{ ".vdx"           , APPLICATION "vnd.visio;version=\"2003-2010\""                                 },
	{ ".vhd"           , TEXT "x-vhdl"                                                                 },
	{ ".vhdl"          , TEXT "x-vhdl"                                                                 },
	{ ".viv"           , VIDEO "vnd-vivo"                                                              },
	{ ".vmdk"          , APPLICATION "x-vmdk"                                                          },
	{ ".vmt"           , APPLICATION "vnd.valve.source.material"                                       },
	{ ".volt"          , TEXT "x-d"                                                                    },
	{ ".vot"           , APPLICATION "x-votable+xml"                                                   },
	{ ".vpb"           , IMAGE "vpb"                                                                   },
	{ ".vsd"           , APPLICATION "vnd.visio"                                                       },
	{ ".vsdm"          , APPLICATION "vnd.ms-visio.drawing.macroenabled.12"                            },
	{ ".vsdx"          , APPLICATION "vnd.visio"                                                       },
	{ ".vssm"          , APPLICATION "vnd.ms-visio.stencil.macroenabled.12"                            },
	{ ".vssx"          , APPLICATION "vnd.ms-visio.stencil"                                            },
	{ ".vstm"          , APPLICATION "vnd.ms-visio.template.macroenabled.12"                           },
	{ ".vstx"          , APPLICATION "vnd.ms-visio.template"                                           },
	{ ".vtf"           , IMAGE "vnd.valve.source.texture"                                              },
	{ ".vtt"           , TEXT "vtt"                                                                    },
	{ ".vwx"           , APPLICATION "vnd.vectorworks;version=\"2015\""                                },
	{ ".w50"           , APPLICATION "vnd.wordperfect;version=\"5.0\""                                 },
	{ ".w51"           , APPLICATION "vnd.wordperfect;version=\"5.1\""                                 },
	{ ".w52"           , APPLICATION "vnd.wordperfect;version=\"5.2\""                                 },
	{ ".warc"          , APPLICATION "warc"                                                            },
	{ ".wast"          , TEXT "x-common-lisp"                                                          },
	{ ".wav"           , AUDIO "x-wav"                                                                 },
	{ ".wbmp"          , IMAGE "vnd.wap.wbmp"                                                          },
	{ ".webapp"        , APPLICATION "x-web-app-manifest+json"                                         },
	{ ".webidl"        , TEXT "x-webidl"                                                               },
	{ ".webm"          , VIDEO "webm"                                                                  },
	{ ".webp"          , IMAGE "webp"                                                                  },
	{ ".wisp"          , TEXT "x-clojure"                                                              },
	{ ".wk1"           , APPLICATION "vnd.lotus-1-2-3;version=\"2.0\""                                 },
	{ ".wk3"           , APPLICATION "vnd.lotus-1-2-3;version=\"3.0\""                                 },
	{ ".wk4"           , APPLICATION "vnd.lotus-1-2-3;version=\"4-5\""                                 },
	{ ".wks"           , APPLICATION "vnd.lotus-1-2-3"                                                 },
	{ ".wma"           , AUDIO "x-ms-wma"                                                              },
	{ ".wmf"           , IMAGE "wmf"                                                                   },
	{ ".wmlc"          , APPLICATION "vnd.wap.wmlc"                                                    },
	{ ".wmls"          , TEXT "vnd.wap.wmlscript"                                                      },
	{ ".wmlsc"         , APPLICATION "vnd.wap.wmlscriptc"                                              },
	{ ".wmv"           , VIDEO "x-ms-wmv"                                                              },
	{ ".woff"          , APPLICATION "font-woff"                                                       },
	{ ".wp4"           , APPLICATION "vnd.wordperfect;version=\"4.0/4.1/4.2\""                         },
	{ ".wpd"           , APPLICATION "vnd.wordperfect"                                                 },
	{ ".wpl"           , APPLICATION "vnd.ms-wpl"                                                      },
	{ ".wrl"           , MODEL "vrml"                                                                  },
	{ ".wsz"           , INTERFACE "x-winamp-skin"                                                     },
	{ ".x3d"           , MODEL "x3d+xml"                                                               },
	{ ".x3f"           , IMAGE "x-raw-sigma"                                                           },
	{ ".xap"           , APPLICATION "x-silverlight-app"                                               },
	{ ".xar"           , APPLICATION "vnd.xara"                                                        },
	{ ".xbm"           , IMAGE "x-xbitmap"                                                             },
	{ ".xc"            , TEXT "x-csrc"                                                                 },
	{ ".xcf"           , IMAGE "xcf"                                                                   },
	{ ".xdm"           , IMAGE "x-xwindowdump;version=\"x10\""                                         },
	{ ".xfdf"          , APPLICATION "vnd.adobe.xfdf"                                                  },
	{ ".xhtml"         , APPLICATION "xhtml+xml"                                                       },
	{ ".xif"           , IMAGE "vnd.xiff"                                                              },
	{ ".xla"           , APPLICATION "vnd.ms-excel;version=\"4.0\""                                    },
	{ ".xlam"          , APPLICATION "vnd.ms-excel.addin.macroenabled.12"                              },
	{ ".xlc"           , APPLICATION "vnd.ms-excel;version=\"3.0\""                                    },
	{ ".xlm"           , APPLICATION "vnd.ms-excel;version=\"3.0\""                                    },
	{ ".xls"           , APPLICATION "vnd.ms-excel"                                                    },
	{ ".xlsb"          , APPLICATION "vnd.ms-excel.sheet.binary.macroenabled.12"                       },
	{ ".xlsm"          , APPLICATION "vnd.ms-excel.sheet.macroenabled.12"                              },
	{ ".xlsx"          , APPLICATION "vnd.openxmlformats-officedocument.spreadsheetml.sheet"           },
	{ ".xltm"          , APPLICATION "vnd.ms-excel.template.macroenabled.12"                           },
	{ ".xltx"          , APPLICATION "vnd.openxmlformats-officedocument.spreadsheetml.template"        },
	{ ".xlw"           , APPLICATION "vnd.ms-excel;version=\"4w\""                                     },
	{ ".xm"            , AUDIO "xm"                                                                    },
	{ ".xmf"           , AUDIO "mobile-xmf"                                                            },
	{ ".xmind"         , APPLICATION "x-xmind"                                                         },
	{ ".xml"           , APPLICATION "xml"                                                             },
	{ ".xmt"           , APPLICATION "mpeg4-iod-xmt"                                                   },
	{ ".xpi"           , APPLICATION "x-xpinstall"                                                     },
	{ ".xpl"           , TEXT "xml"                                                                    },
	{ ".xpm"           , IMAGE "x-xpixmap;version=\"x10\""                                             },
	{ ".xps"           , APPLICATION "oxps"                                                            },
	{ ".xpt"           , APPLICATION "x-sas-xport"                                                     },
	{ ".xq"            , APPLICATION "xquery"                                                          },
	{ ".xquery"        , APPLICATION "xquery"                                                          },
	{ ".xs"            , TEXT "x-csrc"                                                                 },
	{ ".xsd"           , APPLICATION "xml"                                                             },
	{ ".xsl"           , APPLICATION "xml"                                                             },
	{ ".xslfo"         , APPLICATION "xslfo+xml"                                                       },
	{ ".xslt"          , APPLICATION "xslt+xml"                                                        },
	{ ".xsp-config"    , TEXT "xml"                                                                    },
	{ ".xspf"          , APPLICATION "xspf+xml"                                                        },
	{ ".xwd"           , IMAGE "x-xwindowdump"                                                         },
	{ ".xz"            , APPLICATION "x-xz"                                                            },
	{ ".y"             , TEXT "x-yacc"                                                                 },
	{ ".yaml"          , TEXT "x-yaml"                                                                 },
	{ ".yml"           , TEXT "x-yaml"                                                                 },
	{ ".zip"           , APPLICATION "zip"                                                             }
};

static mime_head_map g_signatures_map
{
	{ "\x00\x01\x00\x00\x00"                           , APPLICATION "x-font-ttf"             },
	{ "\x04%!PS-Adobe-"                                , APPLICATION "postscript"             },
	{ "\x1B%-12345X%!PS"                               , APPLICATION "postscript"             },
	{ "\x1F\x8B"                                       , APPLICATION "gzip"/*x-gzip*/         },
	{ "\x1F\x9D"                                       , APPLICATION "x-compress"             },
	{ "!<arch>""\x0A""debian"                          , APPLICATION "x-debian-package"       },
	{ "#! rnews"                                       , MESSAGE "rfc822"                     },
	{ "#!/bin/ash"                                     , APPLICATION "x-shellscript"          },
	{ "#!/bin/bash"                                    , APPLICATION "x-shellscript"          },
	{ "#!/bin/perl"                                    , TEXT "x-perl"                        },
	{ "#!/bin/sh"                                      , APPLICATION "x-shellscript"          },
	{ "#!/usr/bin/env lua"                             , TEXT "x-lua"                         },
	{ "#!/usr/bin/perl"                                , TEXT "x-perl"                        },
	{ "#!/usr/bin/env python"                          , TEXT "x-python"                      },
	{ "#!/usr/bin/env ruby"                            , TEXT "x-ruby"                        },
	{ "#!/usr/bin/env tcl"                             , TEXT "x-tcl"                         },
	{ "#!/usr/bin/env wish"                            , TEXT "x-tcl"                         },
	{ "#!/usr/bin/lua"                                 , TEXT "x-lua"                         },
	{ "#!/usr/bin/python"                              , TEXT "x-python"                      },
	{ "#!/usr/bin/ruby"                                , TEXT "x-ruby"                        },
	{ "#!/usr/bin/tcl"                                 , TEXT "x-tcl"                         },
	{ "#!/usr/bin/wish"                                , TEXT "x-tcl"                         },
	{ "#!/usr/local/bin/lua"                           , TEXT "x-lua"                         },
	{ "#!/usr/local/bin/perl"                          , TEXT "x-perl"                        },
	{ "#!/usr/local/bin/python"                        , TEXT "x-python"                      },
	{ "#!/usr/local/bin/ruby"                          , TEXT "x-ruby"                        },
	{ "#!/usr/local/bin/tcl"                           , TEXT "x-tcl"                         },
	{ "#!/usr/local/bin/wish"                          , TEXT "x-tcl"                         },
	{ "# KDE Config File"                              , APPLICATION "x-kdelnk"               },
	{ "# PaCkAgE DaTaStReAm"                           , APPLICATION "x-svr4-package"         },
	{ "# xmcd"                                         , TEXT "x-xmcd"                        },
	{ "#?RADIANCE\x0A"                                 , IMAGE "vnd.radiance"                 },
	{ "%!PS-Adobe-"                                    , APPLICATION "postscript"             },
	{ "%PDF-"                                          , APPLICATION "pdf"                    },
	{ "*mbx*"                                          , APPLICATION "mbox"                   },
	{ "-----BEGIN PGP MESSAGE-\x00\x1E---"             , APPLICATION "pgp"                    },
	{ "-----BEGIN PGP SIGNATURE-\x00\x03.\""           , APPLICATION "pgp-signature"          },
	{ ".\""                                            , TEXT "troff"                         },
	{ ".sd\x00"                                        , AUDIO "x-dec-basic"                  },
	{ ".snd"                                           , AUDIO "basic"                        },
	{ "/* XPM */"                                      , IMAGE "x-xpmi"                       },
	{ "07070"                                          , APPLICATION "x-cpio"                 },
	{ "1\xBE\x00\x00"                                  , APPLICATION "msword"                 },
	{ "7z\xBC\xAF'\x1C"                                , APPLICATION "x-7z-compressed"        },
	{ "8BPS Adobe Photoshop Image"                     , IMAGE "vnd.adobe.photoshop"          },
	{ ";ELC"                                           , APPLICATION "x-elc"                  },
	{ "<!DOCTYPE HTML"                                 , TEXT "html"                          },
	{ "<?xml version="                                 , APPLICATION "xml"                    },
	{ "<Book "                                         , APPLICATION "x-mif"                  },
	{ "<BookFile"                                      , APPLICATION "x-mif"                  },
	{ "<MML"                                           , APPLICATION "x-mif"                  },
	{ "<MIFFile"                                       , APPLICATION "x-mif"                  },
	{ "<MakerDictionary"                               , APPLICATION "x-mif"                  },
	{ "<Maker"                                         , APPLICATION "x-mif"                  },
	{ "<SCRIBUSUTF8"                                   , APPLICATION "x-scribus"              },
	{ "<map version"                                   , APPLICATION "x-freemind"             },
	{ "@ echo off"                                     , TEXT "x-msdos-batch"                 },
	{ "@echo off"                                      , TEXT "x-msdos-batch"                 },
	{ "@rem"                                           , TEXT "x-msdos-batch"                 },
	{ "@set"                                           , TEXT "x-msdos-batch"                 },
	{ "ADIF"                                           , AUDIO "x-hx-aac-adif"                },
	{ "AT&TFORM"                                       , IMAGE "vnd.djvu"                     },
	{ "Article"                                        , MESSAGE "news"                       },
	{ "BEGIN:VCALENDAR"                                , TEXT "calendar"                      },
	{ "BEGIN:VCARD"                                    , TEXT "x-vcard"                       },
	{ "BM\x00\x03"                                     , IMAGE "x-ms-bmp"                     },
	{ "BZh"                                            , APPLICATION "x-bzip2"                },
	{ "CPC\xB2"                                        , IMAGE "x-cpi"                        },
	{ "CWS"                                            , APPLICATION "x-shockwave-flash"      },
	{ "DICM"                                           , APPLICATION "dicom"                  },
	{ "EIM "                                           , IMAGE "x-eim"                        },
	{ "FGF95a"                                         , IMAGE "x-unknown"                    },
	{ "FLV"                                            , VIDEO "x-flv"                        },
	{ "FOVb"                                           , IMAGE "x-x3f"                        },
	{ "FWS"                                            , APPLICATION "x-shockwave-flash"      },
	{ "Forward to"                                     , MESSAGE "rfc822"                     },
	{ "From:"                                          , MESSAGE "rfc822"                     },
	{ "GIF8"                                           , IMAGE "gif"                          },
	{ "GIF94z"                                         , IMAGE "x-unknown"                    },
	{ "HEADER    "                                     , CHEMICAL "x-pdb"                     },
	{ "HWP Document File"                              , APPLICATION "x-hwp"                  },
	{ "II\x1A\x00\x00\x00HEAPCCDR"                     , IMAGE "x-canon-crw"                  },
	{ "II*\x00"                                        , IMAGE "tiff"                         },
	{ "II*\x00\x10\x00\x00\x00CR"                      , IMAGE "x-canon-cr2"                  },
	{ "IIN1"                                           , IMAGE "x-niff"                       },
	{ "IIRO"                                           , IMAGE "x-olympus-orf"                },
	{ "IIRS"                                           , IMAGE "x-olympus-orf"                },
	{ "LRZI"                                           , APPLICATION "x-lrzip"                },
	{ "LZIP"                                           , APPLICATION "x-lzip"                 },
	{ "MAS_UTrack_V00"                                 , AUDIO "x-mod"                        },
	{ "MM\x00*"                                        , IMAGE "tiff"                         },
	{ "MMOR"                                           , IMAGE "x-olympus-orf"                },
	{ "MOVI"                                           , VIDEO "x-sgi-movie"                  },
	{ "MThd"                                           , AUDIO "midi"                         },
	{ "MZ"                                             , APPLICATION "x-dosexec"              },
	{ "N#! rnews"                                      , MESSAGE "rfc822"                     },
	{ "OTTO"                                           , APPLICATION "vnd.ms-opentype"        },
	{ "OggS"                                           , APPLICATION "ogg"                    },
	{ "P4\x00"                                         , IMAGE "x-portable-bitmap"            },
	{ "P5\x00"                                         , IMAGE "x-portable-greymap"           },
	{ "P6\x00"                                         , IMAGE "x-portable-pixmap"            },
	{ "P7\x00"                                         , IMAGE "x-portable-pixmap"            },
	{ "P7\n#MTPAINT#"                                  , IMAGE "x-portable-multimap"          },
	{ "PBF"                                            , IMAGE "x-unknown"                    },
	{ "PDN3"                                           , IMAGE "x-paintnet"                   },
	{ "PFS1\x0A"                                       , IMAGE "x-pfs"                        },
	{ "PK\x03\x04"                                     , APPLICATION "zip"                    },
	{ "PK\x07\x08PK\x03\x04"                           , APPLICATION "zip"                    },
	{ "PK00PK\x03\x04"                                 , APPLICATION "zip"                    },
	{ "PO^Q`"                                          , APPLICATION "msword"                 },
	{ "Path:"                                          , MESSAGE "news"                       },
	{ "Pipe to"                                        , MESSAGE "rfc822"                     },
	{ "RF64\xFF\xFF\xFF\xFFWAVEds64"                   , AUDIO "x-wav"                        },
	{ "Rar!"                                           , APPLICATION "x-rar"                  },
	{ "Received:"                                      , MESSAGE "rfc822"                     },
	{ "Relay-Version:"                                 , MESSAGE "rfc822"                     },
	{ "Return-Path:"                                   , MESSAGE "rfc822"                     },
	{ "SIT!"                                           , APPLICATION "x-stuffit"              },
	{ "SplineFontDB:"                                  , APPLICATION "vnd.font-fontforge-sfd" },
	{ "StuffIt\x00"                                    , APPLICATION "x-stuffit"              },
	{ "WARC"                                           , APPLICATION "warc"                   },
	{ "Xcur"                                           , IMAGE "x-xcursor"                    },
	{ "Xref:"                                          , MESSAGE "news"                       },
	{ "[KDE Desktop Entry]"                            , APPLICATION "x-kdelnk"               },
	{ "d8:announce"                                    , APPLICATION "x-bittorrent"           },
	{ "drpm"                                           , APPLICATION "x-rpm"                  },
	{ "fLaC"                                           , AUDIO "x-flac"                       },
	{ "filedesc://"                                    , APPLICATION "x-ia-arc"               },
	{ "riff.\x91\xCF\x11\xA5\xD6(\xDB\x04\xC1\x00\x00" , AUDIO "x-w64"                        },
	{ "\rtf\x00"                                       , TEXT "rtf"                           },
	{ "\x89LZO\x00\x0D\x0A\x1A\x0A"                    , APPLICATION "x-lzop"                 },
	{ "\x89PNG\x0D\x0A\x1A\x0A"                        , IMAGE "png"                          },
	{ "\x8AJNG"                                        , VIDEO "x-jng"                        },
	{ "\x8AMNG"                                        , VIDEO "x-mng"                        },
	{ "\x89HDF\x0D\x0A\x1A\x0A"                        , APPLICATION "x-hdf"                  },
	{ "\xCE\xCE\xCE\xCE"                               , APPLICATION "x-java-jce-keystore"    },
	{ "\xFD""7zXZ\x00"                                 , APPLICATION "x-xz"                   },
	{ "\xFE""7\x00#"                                   , APPLICATION "msword"                 },
	{ "\xFF\xD8"                                       , IMAGE "jpeg"                         },
	{ "\xFF\xD8\xFF\xE0"                               , IMAGE "jpeg"                         },
	{ "\xFF\xD8\xFF\xEE"                               , IMAGE "jpeg"                         }
};


static mime_head_map g_signatures_map_offset4
{
	{ "\x0AVersion:Vivo"             , VIDEO "vivo"                     },
	{ "#VRML V"                      , MODEL "vrml"                     },
	{ "-BEGIN PGP PUBLIC KEY BLOCK-" , APPLICATION "pgp-keys"           },
	{ "XPR3"                         , APPLICATION "x-quark-xpress-3"   },
	{ "XPRa"                         , APPLICATION "x-quark-xpress-3"   },
	{ "free"                         , VIDEO "quicktime"                },
	{ "ftyp3g2"                      , VIDEO "3gpp2"                    },
	{ "ftyp3ge"                      , VIDEO "3gpp"                     },
	{ "ftyp3gg"                      , VIDEO "3gpp"                     },
	{ "ftyp3gp"                      , VIDEO "3gpp"                     },
	{ "ftyp3gs"                      , VIDEO "3gpp"                     },
	{ "ftypM4A"                      , AUDIO "mp4"                      },
	{ "ftypM4B"                      , VIDEO "quicktime"                },
	{ "ftypM4P"                      , VIDEO "quicktime"                },
	{ "ftypM4V"                      , VIDEO "mp4"                      },
	{ "ftypavc1"                     , VIDEO "3gpp"                     },
	{ "ftypiso2"                     , VIDEO "mp4"                      },
	{ "ftypisom"                     , VIDEO "mp4"                      },
	{ "ftypjp2"                      , IMAGE "jp2"                      },
	{ "ftypmmp4"                     , VIDEO "mp4"                      },
	{ "ftypmp41"                     , VIDEO "mp4"                      },
	{ "ftypmp42"                     , VIDEO "mp4"                      },
	{ "ftypmp7b"                     , VIDEO "mp4"                      },
	{ "ftypmp7t"                     , VIDEO "mp4"                      },
	{ "ftypqt"                       , VIDEO "quicktime"                },
	{ "idat"                         , VIDEO "quicktime"                },
	{ "idsc"                         , VIDEO "quicktime"                },
	{ "jP"                           , IMAGE "jp2"                      },
	{ "mdat"                         , VIDEO "quicktime"                },
	{ "pckg"                         , APPLICATION "x-quicktime-player" },
	{ "skip"                         , VIDEO "quicktime"                },
	{ "wide"                         , VIDEO "quicktime"                }
};

#define LIST_LEN(x)  ( sizeof(x) / sizeof(x[0]) - 2 )

static std::string s_mime_search(const mime_head_map &mimes, const char *buf, size_t size)
{
	if( size == 0 )
		return "text/plain";
	size_t index = 0;

	for(auto &[key,value] : mimes)
	{
		if( key[index] != buf[index] )
			continue;
		do {
			if( ++index == key.size() )
				return value;

			else if( index == size or key[index] > buf[index] )
				return "unknown";
		}
		while( key[index] == buf[index] );
		break;
	}
	return "unknown";
}

static bool s_is_text_file(std::ifstream &file)
{
	char buf[0x4000] = {0};
	file.read(buf, sizeof(buf));

	auto buf_size = static_cast<size_t>(file.gcount());
	std::string data(buf, buf_size);

	// UTF16 byte order marks
	if( data.starts_with("\xFE\xFF") or data.starts_with("\xFF\xFE") )
		return true;

	// Check the first 128 bytes (see shared-mime spec)
	const char *p = buf;
	const char *e = p + ( 128 < buf_size ? 128 : buf_size );

	for(; p<e; p++)
	{
		if( static_cast<char>(*p) < 32 and *p != 9 and *p != 10 and *p != 13 )
			return false;
	}
	return true;
}

#define BUF_LEN  256

static std::string _mime_from_magic(std::ifstream &file)
{
	if( not file.is_open() )
		return "unknown";

	char buf[BUF_LEN] = {0};
	file.read(buf, BUF_LEN);

	auto size = static_cast<size_t>(file.gcount());
	if( size < BUF_LEN )
	{
		file.seekg(0, std::ios_base::beg);
		if( s_is_text_file(file) )
		{
			file.close();
			return "text/plain";
		}
	}
	auto mime_type = s_mime_search(g_signatures_map, buf, size);
	if( mime_type.empty() and size > 4 )
		mime_type = s_mime_search(g_signatures_map_offset4, buf + 4, size - 4);
	return mime_type;
}

static std::string mime_from_magic(std::string_view file_name)
{
	std::ifstream file(app::absolute_path(file_name).c_str());
	auto mime_type = _mime_from_magic(file);
	file.close();
	return mime_type;
}

void set_suffix_map(suffix_type_map map)
{
	g_suffix_hash = std::move(map);
}

void insert_suffix_map(suffix_type_map map)
{
	for(auto &[key,value] : map)
		g_suffix_hash[std::move(key)] = std::move(value);
}

LIBGS_CORE_API void set_signatures_map(mime_head_map map)
{
	g_signatures_map = std::move(map);
}

LIBGS_CORE_API void insert_signatures_map(mime_head_map map)
{
	for(auto &[key,value] : map)
		g_signatures_map[std::move(key)] = std::move(value);
}

LIBGS_CORE_API void set_signatures_map_offset4(mime_head_map map)
{
	g_signatures_map_offset4 = std::move(map);
}

LIBGS_CORE_API void insert_signatures_map_offset4(mime_head_map map)
{
	for(auto &[key,value] : map)
		g_signatures_map_offset4[std::move(key)] = std::move(value);
}

std::string get_mime_type(std::string_view file_name, bool magic_first)
{
	if( magic_first )
	{
		auto type = mime_from_magic(file_name);
		if( type != "unknown" )
			return type;

		auto name = libgs::file_name(file_name);
		auto pos = name.rfind('.');

		if( pos == std::string::npos )
			return type;

		auto it = g_suffix_hash.find(str_to_lower(name.substr(pos)));
		if( it == g_suffix_hash.end() )
			return type;
		return it->second;
	}
	auto name = libgs::file_name(file_name);
	auto pos = name.rfind('.');

	if( pos == std::string::npos )
		return mime_from_magic(file_name);

	auto it = g_suffix_hash.find(str_to_lower(name.substr(pos)));
	if( it == g_suffix_hash.end() )
		return mime_from_magic(file_name);
	return it->second;
}

bool is_text_file(std::string_view file_name)
{
	std::ifstream file(file_name.data());
	bool res = is_text_file(file);
	file.close();
	return res;
}

bool is_binary_file(std::string_view file_name)
{
	return not is_text_file(file_name);
}

std::string get_text_file_encoding(std::string_view file_name)
{
	std::ifstream file(file_name.data());
	auto res = get_text_file_encoding(file);
	file.close();
	return res;
}

std::string get_mime_type(std::ifstream &file)
{
	return _mime_from_magic(file);
}

bool is_text_file(std::ifstream &file)
{
	if( file.is_open() )
		return s_is_text_file(file);
	return false;
}

bool is_binary_file(std::ifstream &file)
{
	return not is_text_file(file);
}

std::string get_text_file_encoding(std::ifstream &file)
{
	std::string result = "unknown";
	if( not file.is_open() )
		return result;

	char first_byte = 0;
	file.get(first_byte);

	char second_byte = 0;
	if( file.eof() )
		second_byte = 'A';
	else
		file.get(second_byte);

	char third_byte = 0;
	if( file.eof() )
		third_byte = 'A';
	else
		file.get(third_byte);

	char fourth_byte = 0;
	if( file.eof() )
		fourth_byte = 'A';
	else
		file.get(fourth_byte);

	if( static_cast<uint8_t>(first_byte) == 0xEF and static_cast<uint8_t>(second_byte) == 0xBB and static_cast<uint8_t>(third_byte) == 0xBF )
		result = "UTF-8";

	else if( static_cast<uint8_t>(first_byte) == 0xFF and static_cast<uint8_t>(second_byte) == 0xFE )
		result = "UTF-16LE";

	else if( static_cast<uint8_t>(first_byte) == 0xFE and static_cast<uint8_t>(second_byte) == 0xFF )
		result =  "UTF-16BE";

	else if( static_cast<uint8_t>(first_byte) == 0xFF and static_cast<uint8_t>(second_byte) == 0xFE and static_cast<uint8_t>(third_byte) == 0x0 and static_cast<uint8_t>(fourth_byte) == 0x0)
		result = "UTF-32LE";

	else if( static_cast<uint8_t>(first_byte) == 0x00 and static_cast<uint8_t>(second_byte) == 0x00 and static_cast<uint8_t>(third_byte) == 0xFE and static_cast<uint8_t>(fourth_byte) == 0xFF)
		result = "UTF-32BE";

	return result;
}

} //namespace libgs
