/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   constants.hpp                                            :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: lhagemos <lhagemos@student.42berlin.de>    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/22 15:38:09 by lhagemos          #+#    #+#             */
/*   Updated: 2026/02/12 17:34:00 by lhagemos         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef LIB_HPP
# define LIB_HPP

# define MAX 65536
# define SET 1
# define GET 0
# ifndef CGI_TIMEOUT_SEC
#  define CGI_TIMEOUT_SEC 120
# endif
# define MAX_HEADER_SIZE 8192
# define MAX_CGI_BUFFER 65536
# ifndef CLIENT_TIMEOUT_SEC
#  define CLIENT_TIMEOUT_SEC 600
# endif

#endif
