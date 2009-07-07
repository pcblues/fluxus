; an example of the fluxus extrusion tool

(clear)
(clear-colour 0.5)
(define profile (build-circle-profile 12 0.5))

(define width (build-list 100
        (lambda (n) (* n 0.01 (+ 1.5 (cos (* 0.5 n)))))))

(define path (build-list 100
        (lambda (n) (vmul (vector (sin (* 0.2 n)) 0 (cos (* 0.2 n))) (* 0.05 n)))))

(define p (with-state
        (wire-colour 0)
        (colour (vector 1 1 1))   
        (specular (vector 1 1 1))
        (shinyness 20)
        (hint-wire)
        (build-partial-extrusion profile path 10)))

(define (animate)
    (with-primitive p
        (partial-extrude 
            (* (* 0.5 (+ 1 (sin (* 0.2 (time))))) (length path)) 
            profile path width (vector 0 1 0) 0.05)))

(every-frame (animate))

