#!/usr/bin/env racket
#lang typed/racket

#|
Copyright 2026 Danielle Hutzley

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

  http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
|#

; IMPORTS
(require racket/file typed/racket/class)

; CONSTANTS
; The directory containing all templates
(: TEMPLATE-DIR Path)
(define TEMPLATE-DIR
  (string->path "template"))

; The directory containing all append templates
(: APPENDS-DIR Path)
(define APPENDS-DIR
  (build-path TEMPLATE-DIR "appends"))

; The path to the root `meson.build`
(: PARENT-MESON-BUILD Path)
(define PARENT-MESON-BUILD
  (string->path "meson.build"))

; TYPES
(define app-data%
  (class object%
    (super-new)
    (init-field [week : String]
                [problem : String])

    ; GETTERS
    (: get-week (-> String))
    (define/public (get-week) week)

    (: get-problem (-> String))
    (define/public (get-problem) problem)

    ; Get the directory associated with this week
    (: get-week-directory (-> Path))
    (define/public (get-week-directory)
      (build-path (string-append "week" week)))))

;; typed/racket/class is experimental; if class typechecking gets flaky, the fallback is a plain struct + functions (see git history
(define-type AppData%
 (Class (init [week String] [problem String])
        [get-week (-> String)]
        [get-problem (-> String)]
        [get-week-directory (-> Path)]))

; HELPERS
; A simple pipelining operator. Used until I map the Racket repo to Nix, this is necessary if I want to do this
(define-syntax (~> syn)
  (syntax-case syn (lambda)
    [(_ val) #'val]
    [(_ val (lambda (arg-name) body ...) xs ...)
     #'(~> ((lambda (arg-name) body ...) val) xs ...)]
    [(_ _ (lambda args body ...) _ ...)
     (raise-syntax-error '~> "Interleaving lambda must take exactly one argument"
                         #'(lambda args body ...))]
    [(_ val (func args ...) xs ...) #'(~> (func args ... val) xs ...)]
    [(_ val func xs ...) #'(~> (func val) xs ...)]))

; Write a line to a file if it doesn't exist, creating the file if it also doesn't exist
(provide display-to-file-if-nonexistent)
(: display-to-file-if-nonexistent (-> Path String Void))
(define (display-to-file-if-nonexistent file line)
  (let* ([existing-lines (if (file-exists? file)
                             (file->lines file)
                             '())]
         [contains-line (member line existing-lines)])
    (unless contains-line
      (display-to-file (string-append line "\n") file #:exists 'append))))

; Substitute a template in this text
(provide substitute-placeholder)
(: substitute-placeholder (-> Regexp String String String))
(define (substitute-placeholder pattern replacement text)
  (regexp-replace* pattern text replacement))

; Substitute all templates in this config
(provide substitute-placeholders)
(: substitute-placeholders (-> (Instance AppData%) String String))
(define (substitute-placeholders data input-string)
  (~> input-string
      (substitute-placeholder #rx"@@WEEK@@" (send data get-week))
      (substitute-placeholder #rx"@@PROBLEM@@" (send data get-problem))))

; Fill in the path separator into a template
(provide fill-path-separator)
(: fill-path-separator (-> Path Path))
(define (fill-path-separator path)
  (~> (path->string path)
      (lambda (it) (string-split it "@@PATHSEP@@"))
      (lambda (it) (apply build-path (car it) (cdr it)))))

; Get all the template file names from a given path
(provide filter/templates)
(: filter/templates (-> Path (Listof Path)))
(define (filter/templates directory)
  (~> (directory-list directory #:build? #t)
      (filter (compose1 not directory-exists?))
      (map (lambda ([it : Path]) (assert (file-name-from-path it))))))


; Check if the template has any appends
(provide has-append-template?)
(: has-append-template? (-> Path Boolean))
(define (has-append-template? file-name)
  (and (member file-name (filter/templates APPENDS-DIR)) #t))

; ENTRYPOINT
(provide init-templates!)
(: init-templates! (-> (Instance AppData%) Void))
(define (init-templates! app-data)
  (~> (find-system-path 'run-file)
      path-only

      (lambda (it) (cast it Path))

      (lambda (it)
        (if (regexp-match? #rx"^/nix" (path->string it))
            (string->path "assignments")
            it))

      current-directory)


  (let* ([problem-scoped-files (~> (filter/templates TEMPLATE-DIR)
                                   (filter (compose1 not has-append-template?)))]
         [conflicting-files (filter (lambda ([raw-file-name : Path])
                                      (file-exists?
                                        (build-path
                                          (send app-data get-week-directory)
                                          (~> raw-file-name
                                              fill-path-separator
                                              path->string
                                              (substitute-placeholders app-data)
                                              string->path))))
                                    problem-scoped-files)])
   (let ([problem-scoped-count (length problem-scoped-files)]
         [conflicting-count (length conflicting-files)])
     (when (or (= conflicting-count 0) (= conflicting-count problem-scoped-count))
       (display-to-file-if-nonexistent
         PARENT-MESON-BUILD
         (string-append "subdir('" (path->string (send app-data get-week-directory)) "')")))
     (unless (= conflicting-count 0)
       (raise-user-error (string-append "week " (send app-data get-week) " already has scaffolded files:")
                         (map (lambda ([it : Path])
                                (~> it
                                  fill-path-separator
                                  path->string
                                  (substitute-placeholders app-data)))
                              conflicting-files))))
   (for ([template-filename (filter/templates TEMPLATE-DIR)])
     (let* ([processed-path (~> template-filename
                                fill-path-separator
                                path->string
                                (substitute-placeholders app-data)
                                string->path)]
            [target-path (build-path (send app-data get-week-directory) processed-path)]
            [target-exists (file-exists? target-path)]
            [template (~> (build-path TEMPLATE-DIR template-filename)
                          file->string
                          (substitute-placeholders app-data))]
            [append-template-path (build-path APPENDS-DIR template-filename)])
       (make-parent-directory* target-path)

       (with-output-to-file target-path #:exists 'append
         (lambda ()
           (unless target-exists
             (displayln template))
           (when (file-exists? append-template-path)
             (let ([append-template (~> append-template-path
                                        file->string
                                        (substitute-placeholders app-data))])
               (displayln append-template)))))))))

; CLI
; Command line argument parsing
(command-line
  #:program "compsci2-init"
  #:args (week problem)
  (init-templates! (make-object app-data%
                                (cast week String)
                                (cast problem String))))

