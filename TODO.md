Some things that would make MeshUp better:

* apply Model.configuration only when computing Segment::gl_matrix in
  Model::updateSegments() instead of the current mess all over the place
* loading of animations: it is slow for large files and the .csv parsing
  can be sped up.
* reloading: save a timestamp for each loaded file (store it in MeshupApp?
  or Scene?) and only reload files that have changed. One could use QFile
  to query for the timestamp.
* shadows: they are very hacky and only work in a small area. Would also be
  nice to have higher resoultion shadow maps
