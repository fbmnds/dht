(import (chicken foreign))

;; csc -L -lgpiod -I. -o run-led led.scm led.c

;; Inject C prototype so Chicken knows about foo()
#> extern int led(); <#

;; Now define the foreign lambda
(define led
  (foreign-lambda int "led"))

(led)
