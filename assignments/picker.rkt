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
(require typed/racket/system)

; CONSTANTS
; The environment variable for the binary directory
(: BIN-DIR-VAR String)
(define BIN-DIR-VAR "PICKER_BIN_DIR")

(: FZF-ARGS (Listof String))
; NOTE: The space in --prompt is INTENTIONAL and part of the prompt.
(define FZF-ARGS '("--prompt=Run> " "--height=40%" "--reverse"))

; HELPERS
;; General
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

;; Parsing
; Convert a name into alternating runs of digits and non-digits
; This is useful for natural sorting
(provide tokenize-runs)
(: tokenize-runs (-> String (Listof String)))
(define (tokenize-runs src)
  (~> src
      (regexp-match* #rx"[0-9]+|[^0-9]+")
      (map (lambda ([m : (U String (Pairof String Any))])
             (if (pair? m) (car m) m)))))

; Compare a given pair of runs of tokens
(provide token-run<?)
(: token-run<? (-> String String Boolean))
(define (token-run<? x y)
  (define num-x (string->number x))
  (define num-y (string->number y))
  (if (and (real? num-x) (real? num-y))
      (< num-x num-y)
      (string<? x y)))

; Compare the given strings by tokens
(provide natural<?)
(: natural<? (-> String String Boolean))
(define (natural<? x y)
  (let loop ([tokens-x : (Listof String) (tokenize-runs x)]
             [tokens-y : (Listof String) (tokenize-runs y)])
    (cond
      [(null? tokens-x) (not (null? tokens-y))]
      [(null? tokens-y) #f]
      [(string=? (car tokens-x) (car tokens-y))
       (loop (cdr tokens-x) (cdr tokens-y))]
      [else (token-run<? (car tokens-x) (car tokens-y))])))

;; Discovery
(provide list-binaries)
(: list-binaries (-> Path (Listof String)))
(define (list-binaries dir)
  (~> (if (directory-exists? dir) (directory-list dir) '())
      (filter (lambda ([bin : Path])
                (and (file-exists? (build-path dir bin))
                     (memq 'execute (file-or-directory-permissions
                                      (build-path dir bin))))))
      (map path->string)
      (lambda (bin-dir) (sort bin-dir natural<?))))


;; Selection
; Use FZF to pick a file in a directory
(provide pick)
(: pick (-> (Listof String) (Option String)))
(define (pick candidates)
  (define fzf (find-executable-path "fzf"))
  (unless fzf
    (raise-user-error 'compsci2-picker "fzf not found on path, perhaps you meant to run this script through `nix run`?"))
  (define-values (proc out in _err)
    (apply subprocess #f #f (current-error-port) (assert fzf path?) FZF-ARGS))
  (let ([stdout (assert out input-port?)]
        [stdin (assert in output-port?)])
    (for-each (lambda (candidate) (displayln candidate stdin)) candidates)
    (close-output-port stdin)

    (define selection (read-line stdout))
    (subprocess-wait proc)
    (close-input-port stdout)

    (if (string? selection) selection #f)))

; RUNNER
(provide run!)
(: run! (-> (Listof String) Nothing))
(define (run! args)
  (define bin-dir-string (getenv BIN-DIR-VAR))
  (unless bin-dir-string
    (raise-user-error 'compsci2-picker "~a is not set" BIN-DIR-VAR))
  (define bin-dir (string->path (assert bin-dir-string string?)))

  (define candidates (list-binaries bin-dir))
  (when (null? candidates)
    (raise-user-error 'compsci2-picker "no executables found in ~a" bin-dir))

  (define selected (pick candidates))
  (unless selected
    (raise-user-error 'compsci2-picker "no selection made"))

  (define target (build-path bin-dir (assert selected string?)))
  (exit (apply system*/exit-code target args)))

(module+ main
  (run! (vector->list (current-command-line-arguments))))

; TESTS
(module+ test
  (require typed/rackunit)

  (test-case "tokenize splits digit and non-digit runs"
    (check-equal? (tokenize-runs "week10-problem2") '("week" "10" "-problem" "2")))

  (test-case "numeric runs compare as numbers, not strings"
    (check-true (natural<? "week9-problem1" "week10-problem1"))
    (check-false (natural<? "week10-problem1" "week9-problem1")))

  (test-case "problem numbers order naturally too"
    (check-true (natural<? "week1-problem2" "week1-problem10")))

  (test-case "identical names are not less than each other"
    (check-false (natural<? "week1-problem1" "week1-problem1")))

  (test-case "a prefix sorts before its extension"
    (check-true (natural<? "week1" "week1-problem1")))

  (test-case "sorting a realistic set"
    (check-equal?
      (sort '("week10-problem1" "week2-problem10" "week1-problem1" "week2-problem2")
            natural<?)
      '("week1-problem1" "week2-problem2" "week2-problem10" "week10-problem1"))))

