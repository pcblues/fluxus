; a stupid shoot-em-up example for frisbee

(require (lib "frisbee.ss" "fluxus-0.15"))

(define player-pos
  (vec3 (key-control-b #\d #\a 0.01)
        (+ (key-control-b #\w #\s 0.01) 5) 0))

(define fire-time-e (key-time-e #\x))

(define bullets 
  (factory
   (lambda (t)
     (if (< clock (+ t 1000))
         (object
          #:shape 'sphere 
          #:colour (vec3 1 0.5 0.5)
          #:scale (vec3 0.3 0.3 1)
          #:translate (vec3 (vec3-x (value-now player-pos))
                            (vec3-y (value-now player-pos))
                            (- (integral -0.05) 1)))))
   fire-time-e 50))
 
(define enemies
  (factory
   (lambda (t)
     (let loop ((c 10) (l '()))
       (cond ((zero? c) l)
             (else (loop (- c 1) 
                         (cons 
                          (object
                           #:shape 'model
                           #:translate (vec3 (* (- c 5) 10) 0 (- (integral 0.1) 500))
                           #:filename "alien.obj"
                           #:colour (vec3 0.8 1 0.8)) l))))))
   (metro 5) 2))

(define (object-filter proc list-a list-b) 
  (filter-e
   (lambda (a)
     (if (object-struct? (value-now a))
         (foldl
          (lambda (b f)
            (if (and f (object-struct? (value-now b)))
                (proc (value-now a) (value-now b)))
            #t)
          #t
          list-b))
     #t)
  list-a))

(define (sphere-collision-object-filter proc radius list-a list-b)
  (object-filter 
   (lambda (a b)
     (cond ((< (vdist (value-now (object-struct-translate a))
                      (value-now (object-struct-translate b))) radius)
            (proc a b)
            #f)
           (else #t)))
   list-a list-b))

#;(define c-check
  (map-e
   (lambda (e)
     (map-e
      (lambda (enemy-list)
        (sphere-collision-object-filter 
         (lambda (a b)
          (printf "~a ~a ~n" (value-now (object-struct-translate a))
                  (value-now (object-struct-translate b))))
         5
         enemy-list
         bullets))
      enemies))
   (metro 0.1)))
     
#;(define c-check
  (map-e
   (lambda (e)
     (map
      (lambda (bullet)
        (if (object-struct? (value-now bullet))
            (let ((bullet-pos (value-now (object-struct-translate (value-now bullet)))))
              (map 
               (lambda (baddie-list)
                 (map 
                  (lambda (baddie)
                    (if (object-struct? (value-now baddie))
                        (let ((baddie-pos (value-now (object-struct-translate (value-now baddie)))))
                          (if (< (vdist baddie-pos bullet-pos) 2)
                              (printf "HIT!~n")))))
                  baddie-list))
               (value-now enemies)))))
      (value-now bullets)))
   (metro 0.1)))

(scene
 (list
  ; draw the player
  (object
   #:shape 'model
   #:translate player-pos
   #:scale (vec3 0.4 0.4 0.4)
   #:rotate (vec3 0 180 45)
   #:filename "rocket.obj"
   #:colour (vec3 0.8 0.8 1))
      
  bullets   
  enemies
  
  ; draw the ground
  (object
   #:shape 'plane
   #:translate (vec3 0 -60 0)
   #:rotate (vec3 90 0 0)
   #:colour (vec3 0.8 0.8 0.8)
   #:scale (vec3 1000 1000 1000)
   #:hints (list 'unlit))
  ))
