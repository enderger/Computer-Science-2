#!/usr/bin/env guile
!#
; Copyright 2026 Danielle Hutzley
; 
; Licensed under the Apache License, Version 2.0 (the "License");
; you may not use this file except in compliance with the License.
; You may obtain a copy of the License at
; 
;   http://www.apache.org/licenses/LICENSE-2.0
; 
; Unless required by applicable law or agreed to in writing, software
; distributed under the License is distributed on an "AS IS" BASIS,
; WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
; See the License for the specific language governing permissions and
; limitations under the License.

(use-modules (ice-9 ftw)
             (ice-9 textual-ports)
             (ice-9 regex)
             (srfi srfi-13))

; CONSTANTS
(define (file/ base stem)
  (string-append base file-name-separator-string stem))

(define %template-dir "template")
(define %appends-dir (file/ %template-dir "appends"))

; ARGUMENT PARSING
(define args (cdr (program-arguments)))
(unless (= (length args) 2)
  (error "usage: init.scm <week> <day>"))
(define week-string (car args))
(define day-string (cadr args))
(define week-dir (string-append "week" week-string))

; CHANGE WORKING DIRECTORY
(let ((dir (dirname (car (program-arguments)))))
    (chdir (if (string-prefix? "/nix" dir) (file/ "." "assignments") dir)))
        
            
; HELPERS

;; File I/O
(define (slurp path)
  "Read a file completely, this is a helper"
  (call-with-input-file path get-string-all))

(define (spit path text) 
  "Write text to a file, this is a helper"
  (call-with-output-file path (lambda (p) (display text p))))

(define (substitute-placeholders text)
  "Replace all placeholders with their respective values, returning a string"
  (let* ((week-substituted (regexp-substitute/global #f "@@WEEK@@"
                                                     text 
                                                     'pre week-string 'post))
         (day-substituted (regexp-substitute/global #f "@@DAY@@"
                                                    week-substituted 
                                                    'pre day-string 'post))
         (pathsep-substituted 
           (regexp-substitute/global #f "@@PATHSEP@@"
                                     day-substituted
                                     'pre file-name-separator-string 'post))) 
    pathsep-substituted))

(define (template-entries dir)
  "Flat (non-recursive) list of filenames directly under DIR, excluding
  subdirectories (which also excludes '.' and '..', both of which
                        file-is-directory? reports as directories)."
  (filter (lambda (it) (not (file-is-directory? (file/ dir it))))
          (scandir dir)))

(define (make-parents! path)
  "Create a directory and its parents"
  (let ((dir (dirname path)))
    (unless (or (string=? dir ".") (file-exists? dir))
      (make-parents! dir)
      (mkdir dir))))

(define (append-week-if-nonexistent! parent-meson child)
  "Add a child subdirectory to the setup"
  (let* ((existing (if (file-exists? parent-meson) (slurp parent-meson) ""))
         (line (string-append "subdir('" child "')")))
    (unless (string-contains existing line)
      (spit parent-meson (string-append existing line "\n")))))

;; Misc
(define (has-append? raw-name)
  "Returns if a file name has an entry in the appends directory"
  (member raw-name (template-entries %appends-dir)))
                      

; IMPLEMENTATION

;; Check for conflicting files
(define day-scoped (filter (lambda (raw-name) (not (has-append? raw-name)))
                           (template-entries %template-dir))) 
                                   
(define file-conflicts
  (filter (lambda (raw-name)
            (file-exists? (file/ week-dir (substitute-placeholders raw-name))))
          day-scoped))
(define template-count (length day-scoped))
(define conflict-count (length file-conflicts))

(when (or (= conflict-count 0) (= conflict-count template-count))
    (append-week-if-nonexistent! "meson.build" week-dir))

(unless (= existing-count 0)
  (error (string-append "week" week-string "-day"
                        " already has scaffolded files: "
                        (string-join (map (lambda (raw-name) 
                                            (file/ week-dir
                                                   (substitute-placeholders raw-name)))
                                          file-conflicts)
                                     ","))))


;; Copy and template the files
(for-each (lambda (dir)
            (let* ((processed-filename (substitute-placeholders dir))
                   (target (file/ week-dir processed-filename))
                   (template (file/ %template-dir dir))
                   (append-template (file/ %appends-dir dir)))
              (make-parents! target)
              (spit target (substitute-placeholders (slurp template)))
              (when (file-exists? append-template)
                (spit target (string-append 
                               (slurp target)
                               (substitute-placeholders (slurp append-template)))))))
          (template-entries %template-dir))

