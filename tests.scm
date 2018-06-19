(define else true)

(define null? (lambda (x)
  (eq x (quote ())
))

(define and (lambda (lhs rhs)
  (cond
    ((eq? lhs true)
      (cond
        ((eq? rhs true) true)
        (else false)))
    (else false)
)))

(define not (lambda (arg)
  (cond
    ((eq? arg true) false)
    (else true)
)))

