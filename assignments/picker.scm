#!/usr/bin/env guile
!#
(use-modules (ice-9 ftw)
             (ice-9 popen)
             (ice-9 rdelim)
             (ice-9 regex))

(define bin-dir (or (getenv "PICKER_BIN_DIR") (error "PICKER_BIN_DIR not set")))
(define fzf-bin "fzf")   ; resolved via PATH, pinned by the Nix wrapper at build time

(define token-rx (make-regexp "[0-9]+|[^0-9]+"))

(define (tokenize s)
  (let loop ((start 0) (acc '()))
    (let ((m (regexp-exec token-rx s start)))
      (if m
          (loop (match:end m) (cons (match:substring m) acc))
          (reverse acc)))))

(define (token<? a b)
  (let ((na (string->number a)) (nb (string->number b)))
    (if (and na nb)
        (< na nb)
        (string<? a b))))

(define (natural<? a b)
  (let loop ((ta (tokenize a)) (tb (tokenize b)))
    (cond
      ((null? ta) (not (null? tb)))
      ((null? tb) #f)
      ((string=? (car ta) (car tb)) (loop (cdr ta) (cdr tb)))
      (else (token<? (car ta) (car tb))))))

(define (list-executables dir)
  (sort
    (filter (lambda (f) (not (member f (list "." ".."))))
            (or (scandir dir) (list)))
    natural<?))

(define (pick candidates)
  (let* ((tmpdir (string-append (or (getenv "TMPDIR") "/tmp")))
         (tmpl (string-copy (string-append tmpdir "/compsci2-picker-XXXXXX")))
                              
         (out (mkstemp! tmpl)))
    (for-each (lambda (c) (display c out) (newline out)) candidates)
    (close-port out)
    (let* ((cmd (string-append fzf-bin " --prompt='Run> ' --height=40% --reverse < " tmpl))
           (in (open-input-pipe cmd))
           (selection (read-line in)))
      (close-pipe in)
      (delete-file tmpl)
      (if (eof-object? selection) #f selection))))

(define (main args)
  (let ((candidates (list-executables bin-dir)))
    (when (null? candidates)
      (format (current-error-port) "No executables found in ~a~%" bin-dir)
      (exit 1))
    (let ((selected (pick candidates)))
      (unless selected
        (format (current-error-port) "No selection made.~%")
        (exit 1))
      (let ((full-path (string-append bin-dir "/" selected)))
        (apply execlp full-path (cons full-path args))))))

(main (cdr (program-arguments)))
