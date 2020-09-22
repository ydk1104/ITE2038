SELECT X.before_id as first_id, X.before_name as first_name, X.after_name as second_name, Y.after_name as third_name from 
(SELECT T.before_id, T.after_id, T.before_name, name as after_name From
(SELECT before_id, after_id, name as before_name From Evolution, Pokemon Where before_id = Pokemon.id) as T, Pokemon Where after_id =Pokemon.id) as X,
(SELECT T.before_id, T.after_id, T.before_name, name as after_name From
(SELECT before_id, after_id, name as before_name From Evolution, Pokemon Where before_id = Pokemon.id) as T, Pokemon Where after_id =Pokemon.id) as Y
Where X.after_id = Y.before_id
ORDER BY first_id
