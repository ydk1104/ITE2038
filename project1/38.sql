SELECT name From Pokemon where id in
(SELECT after_id From Evolution Where after_id not in
(SELECT before_id From Evolution))
 ORDER BY name
