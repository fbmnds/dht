(import (chicken foreign))
(import miscmacros)

;; riscv-csc-options
;; riscv-csc -L -lgpiod -I. -o run-dht dht.scm dht.c

;; Inject C prototype so Chicken knows about foo()
#> extern int dht(int,float*,float*); <#

;; Now define the foreign lambda
(define dht
  (foreign-lambda int "dht" int (c-pointer float) (c-pointer float)))

(define tries 5)
(define GPIOPin 17)
(define r -1)

(let-location
 ([t float] [h float])
 (while (and (> tries 0)(< r 0))
        (set! r (dht GPIOPin (location h) (location t)))
        (set! tries (- tries 1)))
 (if (zero? r)
     (print "t = " t " h = " h)
     (print "errno " r)))
