/*
    Doc is move outside of main C code fore better redability
*/

static char rpmts_doc[] =
  "A python redredrpm.ts object represents an redRPM transaction set.\n"
  "\n"
  "The transaction set is the workhorse of redRPM. It performs the\n"
  "installation and upgrade of packages. The redrpm.ts object is\n"
  "instantiated by the RedTransactionSet function in the redrpm module.\n"
  "\n"
  "The Redrpm module is a subset of original librpm module taillored \n"
  "for 'redpak' the ultra light weight container model for embedded and vital infrastructures\n"
  "for any further information on redpak check http://redpak.redpesk.bzh\n"
  "\n"
  "The TransactionSet function takes only one arguments (redpath).\n"
  "Redpath should point on the last children of a redpak container node family.\n"
  "Every other argement including signature verification are taken from ${nodepath]/etc/redpack.yaml}:\n"
  "\n"
  ;
