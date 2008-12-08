(defpackage local-search
  (:use cl
	utils
	set)
  (:documentation "Package local-search

Functions
---------
hill-climb
simulated-annealing

Symbols
-------
simple
steepest-ascent
minimize
maximize

Variables
---------
*debug-print*")

  (:export hill-climb
	   simulated-annealing
	   *debug-print*
	   minimize
	   maximize
	   simple
	   steepest-ascent))

(in-package local-search)

(defvar *debug-print* nil)

(defun hill-climb (gen fn &key starting-points starting-function starting-point 
			       (num-restarts 1) (max-num-moves 'infty) (hill-climb-type 'simple)
			       (optimization-type 'maximize))
  "hill-climb NEIGHBOURHOOD-GENERATOR OBJECTIVE-FN &key STARTING-POINTS STARTING-POINT STARTING-FUNCTION (NUM-RESTARTS 1) (MAX-NUM-MOVES 'infty) (HILL-CLIMB-TYPE 'simple) (OPTIMIZATION-TYPE 'maximize)
Implements constant-space hillclimbing maximization over a set of items, with random restarts.  Tie-breaking is arbitrary.  At each step, iterate over the set of neighbours and pick the best one.  Return 1) the optimal item 2) its objective value

NEIGHBOURHOOD-GENERATOR - function that takes in an item and returns the [numbered-set] of neighbouring items.
OBJECTIVE-FN - function that takes in an item and returns the objective value, which is of type 'extended-real
STARTING-POINTS, STARTING-POINT,  STARTING-FUNCTION - exactly one of these must be provided. STARTING-POINT is an item, STARTING-POINTS is a list of items, and STARTING-FUNCTION is a function of zero arguments that returns an item.  If STARTING-POINT is provided, it's always the starting point. If STARTING-POINTS is provided, the starting point for each restart cycles through the list.  If STARTING-FUNCTION is provided, the starting point for each restart is generated by calling the function (which is presumably randomized).
NUM-RESTARTS - the number of times the algorithm restarts.
MAX-NUM-MOVES - a threshold on the number of moves tried per restart.  If a local optimum is not found in this time, just use the last (and therefore best) item seen.
HILL-CLIMB-TYPE - either 'steepest-ascent or 'simple.  Steepest ascent moves to the best neighbour while simple just finds the first neighbour that improves the heuristic.
OPTIMIZATION-TYPE - either 'minimize or 'maximize

Special variables that affect behaviour:
*debug-print*
"
  
  (assert (= 2 (count nil (list starting-points starting-point starting-function))) nil
    "Exactly one of starting-function and starting-points must be provided.")
  (orf starting-function
       (if starting-points
	   (let ((n (length starting-points))
		 (i -1))
	     #'(lambda ()
		 (when (= (incf i) n)
		   (setf i 0))
		 (nth i starting-points)))
	 (constantly starting-point)))
  
  (let ((overall-best nil)
	(overall-best-val (ecase optimization-type (maximize '-infty) (minimize 'infty)))
	(comparison-fn (ecase optimization-type (maximize #'my>) (minimize #'my<)))
	(argopt (ecase optimization-type (maximize #'argmax) (minimize #'argmin))))
    
    ;; Loop over restarts
    (dotimes (i num-restarts (values overall-best overall-best-val))
      (when *debug-print* (format t "~&Starting restart ~a" i))
      (mvbind (best best-val)
	  
	  ;; What happens on a single restart
	  (let* ((current (funcall starting-function))
		 (current-val (funcall fn current))
		 (num-moves 0))
	    (loop 
	      (when *debug-print*
		(format t "~&Move ~a.  Objective value ~a~&~a" num-moves current-val current))
	      (let ((neighbours (funcall gen current)))
		(mvbind (ind local-best-val local-best)
		    (ecase hill-climb-type
		      (simple (do-elements (item neighbours (values current current-val))
				(let ((item-val (funcall fn item)))
				  (when (funcall comparison-fn item-val current-val)
				    (return (values nil item-val item))))))
		      (steepest-ascent (funcall argopt neighbours :key fn)))
		  (declare (ignore ind))
		  (if (and (funcall comparison-fn local-best-val current-val) (my< (incf num-moves) max-num-moves))
		      (setf current local-best
			    current-val local-best-val)
		    (return (values current current-val)))))))
	   
	(when (funcall comparison-fn best-val overall-best-val)
	  (setf overall-best best
		overall-best-val best-val))))))
	     



			   


(defun simulated-annealing (gen energy num-steps &key starting-point (num-restarts 1) (typical-energy-range 1)
						      (init-accept-prob .5) (typical-neighbourhood-size 10)
						      (final-accept-prob .01))
  
  (flet ((compute-temp (prob)
	   (/ (- typical-energy-range) (log (/ prob typical-neighbourhood-size)))))
    (let ((init-temp (compute-temp init-accept-prob))
	  (final-temp (compute-temp final-accept-prob)))
      (let ((temp-decay (expt (/ final-temp init-temp) (/ 1 (1- num-steps))))
	    (overall-best nil)
	    (overall-best-val 'infty))
	(dotimes (i num-restarts (values overall-best overall-best-val))
	  (debug-print 0 "~&Restart ~a.  Current best = ~a with value ~a." i overall-best overall-best-val)
	  
	  ;; Restart i
	  (let ((x starting-point)
		(temp init-temp))
	    
	    (let ((val (funcall energy x))
		  (neighbours (funcall gen x))
		  (best nil)
		  (best-val 'infty))
	    
	      (dotimes (j num-steps)
		(debug-print 0 "~&Step ~a.~&Current element is ~a.~&Temperature is ~a.~&Current best = ~a with value ~a." j x temp best best-val)
	      
		;; Step j of restart
		(let* ((y (prob:sample-uniformly neighbours))
		       (new-val (funcall energy y))
		       (diff (my- new-val val)))
		  (flet ((do-move ()
			   (setf x y val new-val neighbours (funcall gen y))
			   (when (my< val best-val)
			     (setf best-val val best x))))
		    (debug-print 0 "~&Considering ~a with value ~a." y new-val)
		
		    ;; If lower energy, always take it
		    (if (my< diff 0)
			(progn 
			  (debug-print 0 "~&Accepting lower-energy configuration.")
			  (do-move))
		      (let* ((acceptance-prob (myexp (my- 0 (my/ diff temp))))
			     (accepted (< (random 1.0) acceptance-prob)))
			(debug-print 0 "~&Acceptance prob is ~a... " acceptance-prob)
			(if accepted
			    (progn
			      (debug-print 0 "accepted")
			      (do-move))
			  (debug-print 0 "rejected"))))))
		(multf temp temp-decay))
		 
	      (when (my< best-val overall-best-val)
		(setf overall-best best overall-best-val best-val)))))))))
		    
			
		    
		    
		
		
		
	      
	      
	     
	    
	    
	  
	  
	  

	

