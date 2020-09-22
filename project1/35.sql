SELECT name From Pokemon Where id in
(SELECT before_id From Evolution Where before_id > after_id)
ORDER BY name
